		
/*--------------------------------------------------------------------*
 *  dwarfexample:  An example application for extracting debug Information from DWARF format
 *  Author:    Suchitra Venugopal
 *  Date:      12/20/2010
 *--------------------------------------------------------------------*/
 
#include <elf.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h> 
#include <cxxabi.h>


#define DMGL_PARAMS	 (1 << 0)	/* Include function args */
#define DMGL_ANSI	 (1 << 1)	/* Include const, volatile, etc */

// 3rd party libdwarf.
#include "dwarf.h"
#include "libdwarf.h"

#define TRUE 0x01
#define FALSE 0x00


/*--------------------------------------------------------------------*
 *  Macro to create Virtual Address of ELF Section Header             *
 *--------------------------------------------------------------------*/
#define SEC_PTR(index) (((Elf32_Shdr *)(file_bytes+ehdr.e_shoff))+index)
/*--------------------------------------------------------------------*
 *  Macro to create Virtual Address of ELF Program Header             *
 *--------------------------------------------------------------------*/
#define PROGHDR_PTR(index) (((Elf32_Phdr *)(file_bytes + ehdr.e_phoff)) + index)

/*--------------------------------------------------------------------*
 *  Local macros                                                      *
 *--------------------------------------------------------------------*/
#define PTROPR(cast,ptr,oper,addVal)                                  \
     (cast)((unsigned long)(ptr) oper (unsigned long)(addVal))

/*--------------------------------------------------------------------*
 *  Defines used to compute various timing values                     *
 *--------------------------------------------------------------------*/
#define SFL_TSTART           0    /* Start of Processing              */
#define SFL_TINIT            1    /* End of Operation Initialization  */
#define SFL_TLOAD            2    /* End of Memory Map Image          */
#define SFL_TSYMI            3    /* End of Locate STAB and SymTab    */
#define SFL_TSYME            4    /* End of Build Function Locators   */
#define SFL_TFMAP            5    /* End of build SFL Functions       */
#define SFL_TRESU            6    /* End of Update Image Resources    */
#define SFL_TEND             7    /* End of Standard operations       */
#define SFL_TPRTS            8    /* End of diagnostic dumps          */
#define SFL_TMAXCNT          9    /* Total number timings             */
/*--------------------------------------------------------------------*
 *  Timing array subscripts                                           *
 *--------------------------------------------------------------------*/
#define SFL_TSRAW            0    /* Raw timing value                 */
#define SFL_TSSEC            1    /* Timing in seconds                */
#define SFL_TSMIC            2    /* Timing microseconds remainder    */
#define SFL_TSMAXCNT         3    /* Total number slots per timing    */

/*--------------------------------------------------------------------*
 *  LineNumber Printing max value pairs per Print Line                *
 *--------------------------------------------------------------------*/
#define SFL_PL_PAIRS         8    /* Offset/LineNo pairs per line     */

/*--------------------------------------------------------------------*
 *  Layout of a Line number entry.                                    *
 *  Note this structure MUST BE 2 BYTE ALIGNED                        *
 *--------------------------------------------------------------------*/
 #pragma pack(2)
 typedef struct s_DBGLin
 {
  Elf32_Off             Offset;    /* Offset from Function Start      */
  short                 LineNo;    /* 1 based Logical Line Number     */
 } DBG_Line;
 #pragma pack()

/*--------------------------------------------------------------------*
 *  Layout of a Source File Locator Entry.                            *
 *--------------------------------------------------------------------*/
 typedef struct s_srcfile_loc
 {
  struct s_srcfile_loc *pNext;     /* Next or NULL if end-of-list     */
  struct s_func_loc    *pFunHoC;   /* 1st Function in Source File     */
  struct s_func_loc    *pFunToC;   /* Last Function in Source File    */
  const char           *pName;     /* Pointer to actual Name Text     */
  char                 *pMSnkName; /* Member Name in StringSink       */
  Elf32_Word            StabSOIdx; /* Stab Index value N_SOL          */
  short                 RefCnt;    /* number of references            */
  short                 SrcIdx;    /* Logical Index this SrcFile      */
  char                  MemName[28]; /* The Actual member name        */
 } SrcFile_Loc;

/*--------------------------------------------------------------------*
 *  Layout of a Function Locator Entry.                               *
 *--------------------------------------------------------------------*/
 typedef struct s_func_loc
 {
  struct s_func_loc   *pSrcFun;    /* Next Function in SrcFile        */
  unsigned long        SymCRC32;   /* CRC32 of Symbol Name            */
  Elf32_Addr           Soff;       /* Start Offset in executable      */
  Elf32_Addr           Eoff;       /* End Offset in executable        */
  Elf32_Word           Size;       /* Size of function                */
  Elf32_Word           SymIdx;     /* SymTab Index value              */
  Elf32_Word           FnIdx;      /* Stab Index value N_FUN          */
  Elf32_Word           FLidxStab;  /* Stab Index value 1st N_SLINE    */
  Elf32_Word           LLidxStab;  /* Stab Index value last N_SLINE   */
  Elf32_Word           FLidx;      /* DBG_Line Index 1st lineno       */
  Elf32_Word           LLidx;      /* DBG Line Index last lineno      */
  const char          *pSymName;   /* Pointer to Symbol Name text     */
  SrcFile_Loc         *pSrcLoc;    /* Associated Source File Entry    */
  char                *pFSnkName;  /* Function Name in StringSink     */
  char                *pCSnkName;  /* Class Name in StringSink        */
  long                 CntSline;   /* Number of N_SLINE entries       */
  unsigned int         FLOff;      /* Offset in Func of 1st N_SLINE   */
  unsigned int         LLOff;      /* Offset in Func of last N_SLINE  */
  unsigned short       FLNum;      /* Linenum in Func of 1st N_SLINE  */
  unsigned short       LLNum;      /* Linenum in Func of last N_SLINE */
  short                FuncRef;    /* Number of references to Func    */
  char                 SName[164]; /* Un-decorated Symbol Name        */
  char                 Spare[3];
  char                 FuncType;   /* Type Id: S = Symbol             */
                                   /*          F = Function           */
                                   /*          D = Duplicate Function */
                                   /*          A = Alias Function     */
 } Func_Loc;

/*--------------------------------------------------------------------*
 *  This structure identifies the ELF sections that represent         *
 *  one of the debugging groups used to generate SFL data:            *
 *  GROUP                            Base Section   String Section    *
 *  Debugging STABs                  .stab          .stabstr          *
 *  Symbol Table                     .symtab        .symstr           *
 *--------------------------------------------------------------------*/
typedef struct s_sec_list
{
  Elf32_Shdr    *Stb_Hdr;   /* Stab or Stab.Index Section             */
  Elf32_Shdr    *Str_Hdr;   /* Stabstr or Stab.Indexstr section       */
  const char    *Stb_Nam;   /* Stab Section Name                      */
  const char    *Str_Nam;   /* Stab String Section Name               */
  Elf32_Word     Stb_Num;   /* Stab or Stab.index sec number          */
  Elf32_Word     Str_Num;   /* Stabstr or Stab.Indexstr sec numb      */
  long           Sec_Typ;   /* 1 = Section Complete                   */
} sec_list;


// DWARF related
struct srcfilesdata {
    char ** srcfiles;
    Dwarf_Signed srcfilescount;
    int srcfilesres;
};

/*--------------------------------------------------------------------*
 *  *-------------------------------------------------------------*   *
 *  * START of Local ENVCDSNK implementation controls             *   *
 *  * N.B Do not include Framework version of DSNK includes       *   *
 *  *-------------------------------------------------------------*   *
 *--------------------------------------------------------------------*/
#define PST_SUCCESS         0
#define PST_FAILED          -1
#define ENVERR_DSNKVSTG        234     /* DllMap Sink Storage Acq Err */
#define ENVERR_DSNKNFND        235     /* DllMap Sink String not fnd  */
#define ENVERR_DSNKIERR        236     /* DllMap Sink Internal error  */
#define ENVERR_DSNKLISTFULL    237     /* DllMap Sink IdxList full    */
#define ENVERR_DSNKNOSPACE     238     /* DllMap Sink Block full      */

#define ENV_STR_SNKBLK   524288     /* String Sink Block size         */
#define ENV_STR_LST      262144     /* String List initial size       */
#define ENV_STR_SNK     ENV_STR_SNKBLK+ENV_STR_LST

//#define BUFSIZ 999

/*--------------------------------------------------------------------*
 *  All DSink structure are packed on byte boundaries                 *
 *--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*
 *  Memory String Sink Block - Used to contain Unique Text Strings    *
 *--------------------------------------------------------------------*/
 typedef struct ENV_DllSinkBlk
 {
   unsigned char       Sink[ENV_STR_SNKBLK]; /* String Sink Space     */
   unsigned char       List[ENV_STR_LST]; /* String Sink Space        */
 } ENV_DllSinkBlk;

/*--------------------------------------------------------------------*
 *  String Sink Control - Identifies Sink Control areas               *
 *--------------------------------------------------------------------*/
 typedef struct ENV_DllSinkCtl
  {
/*--------------------------------------------------------------------*
 *  DLLMap String Sink Controls                                       *
 *--------------------------------------------------------------------*/
    ENV_DllSinkBlk       *pVOMap;      /* Sink Block Map VirtOrigin   */
    long                  BlkUsed;     /* Total Block bytes used      */
    char                 *pVOSink;     /* 1st sink byte               */
    char                 *pEOSink;     /* End of Sink Set area        */
    char                 *pCOSink;     /* Current Byte in Sink        */
/*--------------------------------------------------------------------*
 *  DLLMap String Sink List Controls                                  *
 *--------------------------------------------------------------------*/
    char                 *pList;       /* Pointer to List start       */
    long                  Size;        /* Size of List in bytes       */
    long                  CMax;        /* Maximum element count       */
    long                  CUsed;       /* Total Active and garbage    */
    long                  CActive;     /* Current active count        */
    long                  EFree;       /* Free element index          */
    long                  EStart;      /* Current 1st element index   */
    long                  EEnd;        /* Current End element index   */
 } ENV_DllSinkCtl;
/*--------------------------------------------------------------------*
 *  DLLMap List Element - Identifies 1 active unique string value     *
 *--------------------------------------------------------------------*/
 typedef struct ENV_DllSinkEle
  {
    char                 *pSText;      /* Ele Key - Ptr to text value */
    long                  UseCnt;      /* Number users of text value  */
  } ENV_DllListEle;
/*--------------------------------------------------------------------*
 *  DLL Diagnostic Map Sink Functions                                 *
 *--------------------------------------------------------------------*/
short      ENV_DSnkInit          (void);

short      ENV_DSnkString        (char          *Txtval,
                                  char         **ppSinkval);

short      ENV_DSnkLocate        (char            *Txtval,
                                  long            *FndEle,
                                  char           **ppSinkval,
                                  ENV_DllListEle **ppInsEle);
static ENV_DllSinkCtl     *pDsc=NULL;  /* Diagnostic Map controls     */
/*--------------------------------------------------------------------*
 *  *-------------------------------------------------------------*   *
 *  * END of Local ENVCDSNK implementation controls               *   *
 *  *-------------------------------------------------------------*   *
 *--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*
 *  A Function entry in the form used by ENVMapDll.                   *
 *--------------------------------------------------------------------*/
typedef struct s_EnvDsnkFunc
   {
    long                     Name;     /* Function Name StringTbl Off */
    long                     Class;    /* C++ Class StringTbl Off     */
    long                     Soff;     /* Starting Offset in Image    */
    long                     Eoff;     /* Ending Offset in Image      */
    long                     FLidx;    /* First Line Number Index     */
    long                     LLidx;    /* Last Line Number Index      */
    long                     UFidx;    /* Unique File Array Index     */
   } ENV_DsnkFunc;

/*--------------------------------------------------------------------*
 *  Layout of the SourceFunctionLineNumber (SFL) resource that        *
 *  is added to the target image.                                     *
 *--------------------------------------------------------------------*/
 typedef struct s_ENV_SFL_ResHeader
 {
/*--------------------------------------------------------------------*
 *  Resource Header                                                   *
 *--------------------------------------------------------------------*/
  long            ResTotSize;           /* Size of entire Resource    */
  long            ResHdrSize;           /* Size of Resource Header    */
/*--------------------------------------------------------------------*
 *  Payload area sizes                                                *
 *  If the field PayPhySize is non-zero then the payload data is      *
 *  Compressed. The data must be uncompressed before attempting       *
 *  to access any of the offset pointers                              *
 *--------------------------------------------------------------------*/
  long            PayLogSize;           /* Uncompressed Payload size  */
  long            PayPhySize;           /* Compressed Payload size    */
/*--------------------------------------------------------------------*
 *  Element descriptors                                               *
 *  N.B. All Element chain Offsets are computed from the start of     *
 *       the logical payload area.                                    *
 *--------------------------------------------------------------------*/
  long            CntFunc;              /* Number of Functions        */
  long            OffFunc;              /* Offset to 1st Function     */
  long            CntUsrc;              /* Number unique SrcFiles     */
  long            OffUsrc;              /* Offset to 1st SrcFile      */
  long            CntLine;              /* Number of Linenumbers      */
  long            OffLine;              /* Offset to LineNumber Array */
/*--------------------------------------------------------------------*
 *  String Table descriptor                                           *
 *  Contains actual text of Function, Class and Source File Names.    *
 *  N.B. All offsets for Text strings are computed from the start     *
 *       of the String Table NOT the Start of the Resource Header.    *
 *--------------------------------------------------------------------*/
  long            StgStbl;              /* String Table total Size    */
  long            OffStbl;              /* Offset to String Table     */
/*--------------------------------------------------------------------*
 *  Check controls for associated ELF image                           *
 *--------------------------------------------------------------------*/
  time_t          LinkTime;             /* Link Time of ELF Image     */
  long            PgmHdrMem;            /* Sum of PgmHeader Memsizes  */
/*--------------------------------------------------------------------*
 *  Reserved area used to pad header to 64 bytes                      *
 *--------------------------------------------------------------------*/
  char            Spare[8];             /* Reserved                   */
/*--------------------------------------------------------------------*
 *  Resource Payload area immediately after Header                    *
 *  Contains variable length arrays of elements and String Table      *
 *  Logical structure is                                              *
 *        1st Function                                                *
 *        nth Function                                                *
 *        1st Unique Source File Array entry                          *
 *        nth Unique Source File Array entry                          *
 *        1st Linenumber array entry                                  *
 *        nth Linenumber array entry                                  *
 *        1st Byte of String Table                                    *
 *--------------------------------------------------------------------*/
 } ENV_SFL_ResHeader;
/*--------------------------------------------------------------------*
 *  ELF Image Mapping Controls                                        *
 *--------------------------------------------------------------------*/
static int           errors = 0;   /* # errors found                 */
static char         *file;         /* current file being processed   */
static unsigned char *file_bytes = (unsigned char *)-1; /* ptr to mmap obj  */
static struct stat   file_stat;     /* file status information       */
static int           file_fd = -1;  /* file descriptor for open file */
static Elf32_Ehdr    ehdr;          /* global header                 */
/*--------------------------------------------------------------------*
 *  Debugging Controls                                                *
 *--------------------------------------------------------------------*/
//static unsigned char fDebug1=FALSE; /* Debugging switch 1            */
//static unsigned char fDebug2=FALSE; /* Debugging switch 1            */
//static unsigned char fDebug3=FALSE; /* Debugging switch 1            */
//static unsigned char fDebug4=FALSE; /* Debugging switch 1            */
/*--------------------------------------------------------------------*
 *  Section Locator and Processing Controls                           *
 *--------------------------------------------------------------------*/
static sec_list      StabSecs[2];   /* sections to print out         */
static SrcFile_Loc  *pSrc1st = NULL;
/*--------------------------------------------------------------------*
 *  Statistics for ELF Image elements                                 *
 *--------------------------------------------------------------------*/
static long          CntSrcLine=0; /* Total Source Line entries     */
static long          FndFunLoc=0;   /* Function Locators created     */
static long          UseFunLoc=0;   /* Locators with STAB references */
static long          SrcForFun=0;   /* Locators with Line numbers    */
static long          UseSrcLoc=0;   /* SrcFile Locators in use       */
/*--------------------------------------------------------------------*
 *  Build Resource control fields                                     *
 *--------------------------------------------------------------------*/
 static ENV_SFL_ResHeader *pResHdr=NULL; /* Pointer to Resource Hdr   */
 static long         BldResStg=0;   /* Size of Entire Resource        */
 static long         BldPayStg=0;   /* Size of Payload                */
 static long         BldHdrStg=0;   /* Size of Resource Header        */
 static long         BldFunStg=0;   /* Size of Function elements      */
 static long         BldSrcStg=0;   /* Size of SrcFile Array          */
 static long         BldLinStg=0;   /* Size of LineNo Array           */
 static long         BldStrStg=0;   /* Size of String Table           */
 static long         BldPgmMem=0;   /* Size of String Table           */
/*--------------------------------------------------------------------*
 *  The arrays of Function Locators 100,00 maximum.                   *
 *  SoffFunLoc  sorted in ascending Start Offset in the ELF Image     *
 *  CRC32FunLoc sorted in ascending Symbol Name CRC Sequence          *
 *--------------------------------------------------------------------*/
static Func_Loc     *SoffFunLoc[131072];
static Func_Loc     *NameFunLoc[131072];
static Func_Loc     *CRC32FunLoc[131072];

//DWARF Related the current CU DIE
 Dwarf_Die CU_die;
 const char  *pStrText;
 char *pFileName;
static Func_Loc     *SortedFunLoc[1024];
SrcFile_Loc *pSrcLoc = NULL;
SrcFile_Loc *pSrcPrv = NULL;
Func_Loc    *pFunLoc = NULL;


//Array for storing the set addresses.
Dwarf_Unsigned setadds[4096];

/*--------------------------------------------------------------------*
 *  The Debugging Line Number array.                                  *
 *--------------------------------------------------------------------*/
 #pragma pack(2)
 static DBG_Line     DBGLines[524288];
 #pragma pack()
/*--------------------------------------------------------------------*
 *  Function prototypes                                               *
 *--------------------------------------------------------------------*/
const char *get_section_name(Elf32_Word sec_index);

void build_sec_list(Elf32_Word       sec_num,
                    const char      *sec_name,
                    const char      *str_name,
                    sec_list        *pSecLst);

const char * elf_string(Elf32_Word sec_index,
                        Elf32_Word byte_off,
                        Elf32_Word byte_rel);

	void build_func_loc(Elf32_Shdr *sec_hdr,
				   long StbVal);

	void build_resource_object(void);

	void sort_func_loc_soff(Func_Loc *LSet[], long left, long right);

	int  CmpSoff(const void *a, const void *b);

	int  CmpName(const void *a, const void *b);

	int  CmpCRC32(const void *a, const void *b);

	unsigned long crc32(const unsigned char *buf, unsigned int len);

	//All DWARF Related
	int CmpLineNum(const void *a, const void *b);

	void build_sfl_dwarf(Dwarf_Debug dbg,Elf32_Shdr *sec_hdr);

	void process_die_data(Dwarf_Debug dbg, Dwarf_Die print_me,struct srcfilesdata *sf);

	void get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,int in_level, struct srcfilesdata *sf);

	void resetsrcfiles(Dwarf_Debug dbg,struct srcfilesdata *sf);

	void print_comp_dir(Dwarf_Debug dbg,Dwarf_Die die,int level, struct srcfilesdata *sf); // not used.

	void get_highlowpc_linkagename(Dwarf_Debug dbg,Dwarf_Die die, 
	    struct srcfilesdata *sf,Dwarf_Addr *lowpc1,Dwarf_Addr *highpc1,char **HPLinkagename,short *LinkNameGot);

	void get_highlowpc_procedurename(Dwarf_Debug dbg,Dwarf_Die die,
	    struct srcfilesdata *sf,Dwarf_Addr *lowpc1,Dwarf_Addr *highpc1,char *proc_name_buf, int proc_name_buf_len);

	void get_number(Dwarf_Attribute attr,Dwarf_Unsigned *val);

	void get_addr(Dwarf_Attribute attr,Dwarf_Addr *val);


	void convert_to_upper(char *StrText);

	Func_Loc* check_crc_match(unsigned long SymCRC32);

	int search_function_in_file(char **pFileName);


	int process_subprogram(Dwarf_Debug dbg,Dwarf_Addr lowpc1,Dwarf_Addr highpc1,
			     char **HPLinkagename,short LinkNameGot,char **name,Dwarf_Signed sectionCount); 


	/*--------------------------------------------------------------------*
	 *  Function: main                                                    *
	 *  Execution control and setup                                       *
	 *--------------------------------------------------------------------*/
	int main(int argc,
		 char *argv[])
	{
	 Elf32_Word    i;
	 const char   *pName;
	 SrcFile_Loc  *pSrcLoc;
	 Func_Loc     *pFunLoc;
	 DBG_Line     *pLine;
	/*--------------------------------------------------------------------*
	 *  Timing Controls                                                   *
	 *--------------------------------------------------------------------*/
	 //clock_t       TimingData[SFL_TMAXCNT][SFL_TSMAXCNT];
	 clock_t       T3Time;
	 struct tm    *dt;            /* Time structure              */
	 struct tms    RTime;
	/*--------------------------------------------------------------------*
	 *  Argument processing Controls                                      *
	 *--------------------------------------------------------------------*/
	 int           argnum;
	 int           letter;
	 int           ch;
	/*--------------------------------------------------------------------*
	 *  Structure formating control                                       *
	 *--------------------------------------------------------------------*/
	 long          dj,dk,dl,dm,dn;
	 short         j,k;
	 unsigned char fDumpSum=FALSE;
	 unsigned char fDumpSym=FALSE;
	 unsigned char fDumpSrc=FALSE;
	 unsigned char fDumpSrcFiles=FALSE;
	 unsigned char fDumpFun=FALSE;
	 unsigned char fDumpAllFuns=FALSE;

	 unsigned char fDumpTim=FALSE;
	 unsigned char fDumpBlk=FALSE;
	 char          LinkTime[30];

	/*--------------------------------------------------------------------*
	 *  DWARF processing controls                                         *
	 *--------------------------------------------------------------------*/
	 Dwarf_Debug dbg = 0;
	 Dwarf_Error error;
	 Dwarf_Handler errhand = 0;
	 Dwarf_Ptr errarg = 0;
	 int res = DW_DLV_ERROR;

	 extern char *optarg;
	 extern int optind, optopt, opterr;

	 char *ifile;

	/*--------------------------------------------------------------------*
	 *  Initialize the Timing Control Area and record Start Time          *
	 *--------------------------------------------------------------------*/
	// memset(&TimingData,0x00,sizeof(TimingData));
	// TimingData[SFL_TSTART][SFL_TSRAW] = times(&RTime);
	/*--------------------------------------------------------------------*
	 *  Process any input options                                         *
	 *--------------------------------------------------------------------*/
	 while ((letter = getopt(argc,argv,"muslf:")) != EOF)
	   {
	    switch (letter)
	      {
		 
		   case 'm':
				   fDumpSum = TRUE;
				   break;
	       case 'u': 
				 fDumpSrc = TRUE; 
				 break;
	       case 's': 
				 fDumpSrcFiles = TRUE; 
				 break;
	       case 'l': 
				 fDumpAllFuns = TRUE; 
				 break;
		   case 'f':   
			 ifile = optarg;
				// optind++;
				 fDumpFun = TRUE;
				 break;
	       default: errors = 1; break;

	      }
	   }
	//   printf("\n optind=%d, argc=%d",optind,argc);
	 if (errors || optind >= argc)
	   {
	    fprintf (stderr, "Calling sequence:\n\n");
	    fprintf (stderr, "dwarfexample -f func_name [-musl] input_binary_filename > outfilename\n");
	    fprintf (stderr, "-m Dump Summary statistics.\n");
	    fprintf (stderr, "-u Dump Unique Source Files.\n");
	    fprintf (stderr, "-s Dump All Src file details\n");
	    fprintf (stderr, "-l Dump All function details\n");


	    return(1);
	   }
	 //TimingData[SFL_TINIT][SFL_TSRAW] = times(&RTime);
	 file = argv[optind];
	 /* Map in the file. */
	 //printf("\nsize = %lu", file_stat.st_size);
	 if (stat(file, &file_stat) < 0 ||
	     (file_fd = open(file, O_RDONLY)) < 0 ||
	     (file_bytes = (unsigned char *) mmap((caddr_t)0,
					    (size_t)file_stat.st_size,
					    PROT_READ,
					    MAP_PRIVATE,
					    file_fd,
					    (off_t)0)) == (unsigned char *)-1)
	   {
	    perror(file);
	    errors = 2;
	    goto Exit_Map_Err;
	   }
	 ehdr = *(Elf32_Ehdr *)file_bytes;
	 if (file_stat.st_size < sizeof (ehdr) || memcmp(ehdr.e_ident,"\177ELF",4) != 0)
	   {
	    fprintf(stderr, "%s is not an ELF file.\n", file);
	    errors = 3;
	    goto Exit_Map_Err;
	   }
	 if (ehdr.e_ehsize != sizeof (ehdr))
	   {
	    fprintf(stderr, "%s ELF Header is %lu bytes, expected %lu\n", file,
		    ehdr.e_ehsize,sizeof(ehdr));
	    errors = 4;
	    goto Exit_Map_Err;
	   }
	 //TimingData[SFL_TLOAD][SFL_TSRAW] = times(&RTime);
	/*--------------------------------------------------------------------*
	 *  Init .stab and .symtab Section Lists                              *
	 *--------------------------------------------------------------------*/
	 memset((void *) &StabSecs[0],0x00,sizeof(sec_list));
	 memset((void *) &StabSecs[1],0x00,sizeof(sec_list));
	 memset((void *) &CRC32FunLoc[0],0x00,sizeof(CRC32FunLoc));
	/*--------------------------------------------------------------------*
	 *  Scan the ELF file looking for stab and symbol sections            *
	 *--------------------------------------------------------------------*/
	 //suchi for debug
	 Elf32_Half          e_type;
	 Elf32_Half          e_machine;
	 e_type = ehdr.e_type;
	 e_machine = ehdr.e_machine;
	 for (i = 0; i < ehdr.e_shnum; i++)
	   {
	    pName = NULL;
	    pName = get_section_name(i);
		// Debug stmts - suchi
//		printf("\nSection Name = %s", pName);
	    if (pName)
	      {
	/*--------------------------------------------------------------------*
	 *  ELF Symbol Table                                                  *
	 *--------------------------------------------------------------------*/
	       if (strcmp(pName,".symtab") == 0)
		   {
		 build_sec_list(i,pName,".strtab",&StabSecs[1]);
		   }
	     }
	   }

	 /*--------------------------------------------------------------------*
	 *  Build Function locators from Symbol Table entries                 *
	 *--------------------------------------------------------------------*/

	if(StabSecs[1].Sec_Typ == 1){
		build_func_loc(StabSecs[1].Stb_Hdr,1); // suchi
	}
	if (FndFunLoc < 1)
	{
	      fprintf(stderr, "%s has no Function Entries in Symbol Table\n", file);
	      errors = 5;
	}
	else
	{
			/*--------------------------------------------------------------------*
			 *  Build SFL data if have valid sections                             *
			 *--------------------------------------------------------------------*/
			 // Initialise the DWARF
			res = dwarf_init(file_fd,DW_DLC_READ,errhand,errarg, &dbg,&error);

			if(res != DW_DLV_OK) {
				printf("Giving up, cannot do DWARF processing\n");
			exit(1);
			}
			// Now Read the DWARF and store values
			build_sfl_dwarf(dbg,StabSecs[0].Stb_Hdr);

			res = dwarf_finish(dbg,&error);
		    if(res != DW_DLV_OK) {
			    printf("dwarf_finish failed!\n");
			}
	}
	/*--------------------------------------------------------------------*
	 *  Generate a Resource Object if Functions Found                     *
	 *--------------------------------------------------------------------*/
	if (SrcForFun > 0) build_resource_object();

	/*--------------------------------------------------------------------*
	 * Ask for Function name and give the details of the same             *
	 *--------------------------------------------------------------------*/
	 if(fDumpFun)
	 {
	   int funFound = 0;
	   char *pch;
	   char *strfunc;
	   int classFound = 0;
	   printf("\n Function name the user requested is : %s \n",ifile);   
	   if( strchr(ifile,':'))
	      classFound = 1; // class name is provided with the function name for searching. then search the full name alone
	   // Now search for this function in the array of functions and get the file name and line numbers
	   for (dj = 0, dk = 0; dj < FndFunLoc; dj++)
	      {
	       pFunLoc = SoffFunLoc[dj];
	       if (pFunLoc->FuncRef == 0 || pFunLoc->pSrcLoc == NULL || pFunLoc->CntSline == 0) continue;
		   if(classFound) // search full string
		   {
		      if(strcmp(ifile,pFunLoc->SName)==0)
			  {
			funFound = 1;
				printf("    Class:%s\n",pFunLoc->pCSnkName != NULL ? pFunLoc->pCSnkName : "(null)");
				printf("    Source File:%s\n",pFunLoc->pSrcLoc->pMSnkName);
				printf("    First Linenum:%-5.5hu\n",pFunLoc->FLNum);
				printf("    Last Linenum:%-5.5hu\n",pFunLoc->LLNum);
			printf("\n--------------------------------------------------------------------\n");
		  }
		   }
	       else
		   {
		    strfunc = pFunLoc->SName;
				pch=strchr(pFunLoc->SName,':');
				if(pch) // : is found, get the function name alone
				{
					 strfunc = pch+2;
				}
				if(strcmp(ifile,strfunc) ==0) 
			{
					funFound = 1;
					printf("    Class:%s\n",pFunLoc->pCSnkName != NULL ? pFunLoc->pCSnkName : "(null)");
					printf("    Source File:%s\n",pFunLoc->pSrcLoc->pMSnkName);
					printf("    First Linenum:%-5.5hu\n",pFunLoc->FLNum);
					printf("    Last Linenum:%-5.5hu\n",pFunLoc->LLNum);
				printf("\n--------------------------------------------------------------------\n");
				}
		   }
	      }
		  if(!funFound)
		    printf("The function is not found in this file!!!\n");
	  
	 }

	/*--------------------------------------------------------------------*
	 *  Write some statistics about this execution.                       *
	 *--------------------------------------------------------------------*/
	 if (fDumpSum == TRUE)
	   {
	    dt = gmtime(&file_stat.st_mtime);
	    strftime(LinkTime,20,"%m/%d/%Y %H:%M:%S",dt);
		printf("<========== Statistics about this execution ==========>\n");
	    printf("         Input File:%s\n",file);
	    printf("Processing complete:%s\n",file); //basename(file));
	    printf("     Last Link Time:%s\n",LinkTime);
	    printf("            Symbols:%7.7ld\n",FndFunLoc);
	    printf("Unique Source Files:%7.7ld\n",UseSrcLoc);
	    printf("          Functions:%7.7ld\n",SrcForFun);
	   // printf("       Line Numbers:%7.7ld\n",CntSrcLine);

	   }

	/*--------------------------------------------------------------------*
	 *  Dump Unique Source File List                                      *
	 *--------------------------------------------------------------------*/
	 if (fDumpSrc == TRUE)
	   {
	    printf("<========== Unique Source Files in this binary ==========>\n");
	    printf("Idx  Name\n");
	    for (pSrcLoc = pSrc1st; pSrcLoc != NULL; pSrcLoc = pSrcLoc->pNext)
	      {
	       printf("%-4.4ld  %s\n",
		      pSrcLoc->SrcIdx+1,
		      pSrcLoc->MemName);
	      }
	    printf("\n");
	   }


	// Debug stmt - print the Source file list
	if(fDumpSrcFiles){
		int num = 1;
		printf("\nSource file list");
		printf("\n================");
		for (pSrcLoc = pSrc1st; pSrcLoc != NULL; pSrcLoc = pSrcLoc->pNext)
		{
			printf("\n%d. File name = %s",num,pSrcLoc->MemName);
			num++;
			printf("\n   No of Functions = %d\n",pSrcLoc->RefCnt);
			printf("\n\tFunctions in %s are : ",pSrcLoc->MemName );
			printf("\n\t-----------------------------");
			for(k=0;k<pSrcLoc->RefCnt;k++){ //-1
					printf("\n\t%d. %s",k+1,pSrcLoc->pFunHoC->SName);
					pSrcLoc->pFunHoC = pSrcLoc->pFunHoC->pSrcFun;
			}
		    printf("\n*****************************************************");
		}
	}
	if(fDumpAllFuns){
		printf("\n Function list");
		printf("\n =============\n\n");
		for (dj = 0, dk = 0; dj < FndFunLoc; dj++)
	      {
	       pFunLoc = SoffFunLoc[dj];
	       if (pFunLoc->FuncRef == 0 || pFunLoc->pSrcLoc == NULL || pFunLoc->CntSline == 0) continue;
	       printf("%-6.6ld. %s\n",
		      ++dk,
		      pFunLoc->SName); // Fn name
	       printf("           Name:%s\n",pFunLoc->pFSnkName);
	       printf("          Class:%s\n",pFunLoc->pCSnkName != NULL ? pFunLoc->pCSnkName : "(null)");
	       printf("           Type:%c\n",pFunLoc->FuncType);
	       printf("    Source File:%s\n",pFunLoc->pSrcLoc->pMSnkName);
	       printf("  First Linenum:%-5.5hu\n",pFunLoc->FLNum);
	       printf("   Last Linenum:%-5.5hu\n",pFunLoc->LLNum);
		   printf("\n--------------------------------------------------------------------\n");


	      }
	}

	 Exit_Map_Err:
	 if (file_bytes != (unsigned char *)-1)
	   munmap ((caddr_t)file_bytes, (size_t)file_stat.st_size);
	 if (file_fd >= 0) close (file_fd);
	 return(errors);
	}


	/*--------------------------------------------------------------------*
	 *  Function: build_func_loc                                          *
	 *  Build the Function Locator List                                   *
	 *--------------------------------------------------------------------*/
void build_func_loc(Elf32_Shdr *sec_hdr,
                    long StbVal)
{
 Elf32_Word    size = sec_hdr->sh_size;
 Elf32_Sym    *ptr = (Elf32_Sym *)(file_bytes + sec_hdr->sh_offset);
 Elf32_Sym    *endptr = (Elf32_Sym *)((char *)ptr + size);
 Func_Loc     *pFunLoc;
 const char   *pSymName;
 char         *pCppName;
 unsigned int  SymNameLen;
 int           sym_num = 0;
 char         *result;     /* Returned demangled string from         */ 
                           /* cplus_demangle Must be freed.          */                
/*--------------------------------------------------------------------*
 *  Scan the Symbol Table looking for Defined functions               *
 *--------------------------------------------------------------------*/
 ptr = (Elf32_Sym *)(file_bytes + sec_hdr->sh_offset);

//printf("\nFunction names in symbol table are :");
 while (ptr < endptr)
   {
    if (ELF32_ST_TYPE(ptr->st_info) == STT_FUNC && (int)ptr->st_shndx != (int) SHN_UNDEF)
      {
     
       pFunLoc = (Func_Loc *) malloc(sizeof(Func_Loc));
       memset((void *) pFunLoc,0x00,sizeof(Func_Loc));
       pSymName = elf_string(sec_hdr->sh_link,ptr->st_name,0);
     
       pFunLoc->Size = ptr->st_size;
       pFunLoc->Soff = ptr->st_value;
       pFunLoc->Eoff = pFunLoc->Soff + (pFunLoc->Size - 1);
       pFunLoc->SymIdx = sym_num;
       pFunLoc->pSymName = pSymName;
       pFunLoc->FuncType = 'S';


       SymNameLen = (unsigned int) strlen(pSymName);
       pFunLoc->SymCRC32 = crc32((const unsigned char *) pSymName,SymNameLen);

	   //suchi for IA
	   size_t length = 256;
	   int status = 0;
	   char* retname=0;
	   if(status !=0 )
		   printf("\nDemangling failed");
	   //End 
       if (retname != NULL)  
         {
          memcpy(pFunLoc->SName, retname, 163);
           free(retname);
          if (strchr(pFunLoc->SName,'~'))
            {
             if ((pCppName = strchr(pFunLoc->SName,' ')) != NULL) *pCppName = 0x00;
            }
          if ((pCppName = strchr(pFunLoc->SName,'(')) != NULL) *pCppName = 0x00;
         }
       else
       {
          strncpy(pFunLoc->SName, pSymName, SymNameLen);
       }
       memcpy((void *) &pFunLoc->SName[160],(void *) &pFunLoc->Soff,4);

       SoffFunLoc[FndFunLoc] = pFunLoc;
       CRC32FunLoc[FndFunLoc] = pFunLoc;
       NameFunLoc[FndFunLoc] = pFunLoc;
       FndFunLoc++;
      }
    sym_num++;
    ptr++;
   }
/*--------------------------------------------------------------------*
 *  If Entries were found sort the array into ascending               *
 *  offset order.                                                     *
 *--------------------------------------------------------------------*/
 if (FndFunLoc > 1)
   {
   qsort((void *)SoffFunLoc, FndFunLoc, sizeof(struct s_func_loc *), CmpSoff);
   qsort((void *)NameFunLoc, FndFunLoc, sizeof(struct s_func_loc *), CmpName);
   qsort((void *)CRC32FunLoc, FndFunLoc, sizeof(struct s_func_loc *), CmpCRC32);
   }
}

/*--------------------------------------------------------------------*
 *  Function: build_resource_object                                   *
 *  Create Resource Object containing the SFL Payload                 *
 *--------------------------------------------------------------------*/
void build_resource_object(void)
{
/*--------------------------------------------------------------------*
 *  Build Resource control fields                                     *
 *--------------------------------------------------------------------*/
 char                      *pBldPay;     /* Start of Payload area     */
 ENV_DsnkFunc              *pBldFun;     /* Start of Functions        */
 long                      *pBldSrc;     /* Start of SrcFile Array    */
 char                      *pBldLin;     /* Start of LineNo Array     */
 char                      *pBldStr;     /* Start of String Table     */
 char                      *pStrTbl;     /* True StringTable VO..     */
 ENV_DsnkFunc              *pBldCurFun;  /* Current Function          */
 long                      *pBldCurSrc;  /* Current SrcFile Array     */
/*--------------------------------------------------------------------*
 *  Local Work variables                                              *
 *--------------------------------------------------------------------*/
 Elf32_Phdr                *prog_hdr;
 Elf32_Word                 ph_index;
 Elf32_Addr                 Logical_Load=0xFF0000FF;
 SrcFile_Loc               *pSrcLoc;
 Func_Loc                  *pFunLoc;
 char                      *pStrPtr1;
 char                      *pStrPtr2;
 long                       i;
 int                        Out_Fd;
 short                      LclLen;
 short                      MyRet;
 char                       LclCls[160];

/*--------------------------------------------------------------------*
 *  Initialize the String Sink                                        *
 *--------------------------------------------------------------------*/
 if ((MyRet = ENV_DSnkInit()) != PST_SUCCESS)
   {
    fprintf(stderr, "DLL Sink failed to Initialize rc=%hd\n",MyRet);
    errors = 6;
    return;
   }
/*--------------------------------------------------------------------*
 *  Add all Source Member names to String Sink                        *
 *--------------------------------------------------------------------*/
  for (pSrcLoc = pSrc1st; pSrcLoc != NULL; pSrcLoc = pSrcLoc->pNext)
    {
     if ((MyRet = ENV_DSnkString(pSrcLoc->MemName,&pSrcLoc->pMSnkName)) != PST_SUCCESS)
       {
        fprintf(stderr, "DLL Sink Add failed rc=%hd SrcFile Member=%s\n",MyRet,pSrcLoc->MemName);
        errors = 7;
        return;
       }
   }
/*--------------------------------------------------------------------*
 *  Add all Function and Class Names to String Sink                   *
 *--------------------------------------------------------------------*/
  for (i = 0; i < FndFunLoc; i++)
    {
     pFunLoc = SoffFunLoc[i];
     if (pFunLoc->CntSline < 1) continue;
     pStrPtr1 = pFunLoc->SName;
/*--------------------------------------------------------------------*
 *  C++ Class::Member                                                 *
 *--------------------------------------------------------------------*/
     if ((pStrPtr2 = strchr(pFunLoc->SName,':')) != NULL)
       {
        LclLen = pStrPtr2 - pStrPtr1;
        memcpy(LclCls,pStrPtr1,LclLen);
        LclCls[LclLen] = 0x00;
        pStrPtr2 += 2;
        if ((MyRet = ENV_DSnkString(LclCls,&pFunLoc->pCSnkName)) != PST_SUCCESS)
          {
           fprintf(stderr, "DLL Sink Add failed rc=%hd C++ Class=%s\n",MyRet,LclCls);
           errors = 8;
           return;
          }
        if ((MyRet = ENV_DSnkString(pStrPtr2,&pFunLoc->pFSnkName)) != PST_SUCCESS)
          {
           fprintf(stderr, "DLL Sink Add failed rc=%hd C++ Function=%s\n",MyRet,pStrPtr2);
           errors = 9;
           return;
          }
       }
/*--------------------------------------------------------------------*
 *  C Function Name                                                   *
 *--------------------------------------------------------------------*/
     else
       {
        if ((MyRet = ENV_DSnkString(pFunLoc->SName,&pFunLoc->pFSnkName)) != PST_SUCCESS)
          {
           fprintf(stderr, "DLL Sink Add failed rc=%hd C Function=%s\n",MyRet,pFunLoc->SName);
           errors = 10;
           return;
          }
       }
   }
/*--------------------------------------------------------------------*
 *  Convert the Source,Function,Linenumber structures into a          *
 *  an offset based based set of structures that are contained in     *
 *  contiguous memory. This area will be written to the ".sfl" file   *
 *  associated with this image.                                       *
 *--------------------------------------------------------------------*/
// Accumulate various Payload sizes
  BldFunStg = SrcForFun * sizeof(ENV_DsnkFunc);
  BldSrcStg = UseSrcLoc * 4;  /* Sizeof a pointer to char         */
  BldLinStg = CntSrcLine * sizeof(DBG_Line);
  BldStrStg = pDsc->BlkUsed;
  pStrTbl = pDsc->pVOSink;    /* Real String Table Address  */
  BldPayStg = BldFunStg + BldSrcStg + BldLinStg + BldStrStg;
/*--------------------------------------------------------------------*
 *  Compute Size of Resource rounded to a 64 byte multiple            *
 *--------------------------------------------------------------------*/
  BldHdrStg = sizeof(ENV_SFL_ResHeader); /* Size of Header            */
  BldResStg = ((BldHdrStg + BldPayStg) + 63) & -64;
/*--------------------------------------------------------------------*
 *  Acquire storage for Resource                                      *
 *--------------------------------------------------------------------*/
  if ((pResHdr  = (ENV_SFL_ResHeader *) calloc(1,BldResStg)) == NULL)
    {
     fprintf(stderr, "Could not Acquire Resource Storage size=%ld\n",BldResStg);
     return;
    }
/*--------------------------------------------------------------------*
 *  Complete the static fields in the Resource Header                 *
 *--------------------------------------------------------------------*/
 pResHdr->ResTotSize = BldResStg;
 pResHdr->ResHdrSize = BldHdrStg;
 pResHdr->PayLogSize = BldPayStg;
 pResHdr->PayPhySize = 0;            /* Ensure uncompressed */
 pResHdr->CntFunc = SrcForFun;
 pResHdr->CntUsrc = UseSrcLoc;
 pResHdr->CntLine = CntSrcLine;
 pResHdr->StgStbl = BldStrStg;
/*--------------------------------------------------------------------*
 *  The ELF image modification time is the assumed Link Edit time     *
 *  This is used by ENV_MapDll o set the value displayed in the       *
 *  Trace File Header for the ELF Image.                              *
 *--------------------------------------------------------------------*/
 pResHdr->LinkTime = file_stat.st_mtime;
/*--------------------------------------------------------------------*
 *  The Sum of the ELF Program Header entry p_memsz fields is         *
 *  saved for later comparision with the ELF Image being              *
 *  processed by ENV_MapDll at run-time. If the values are not        *
 *  the same than it is assumed that the ELF Image and the .sfl       *
 *  file are no in sync.                                              *
 *--------------------------------------------------------------------*/
  for (ph_index = 0; ph_index < ehdr.e_phnum; ph_index++)
    {
     prog_hdr = PROGHDR_PTR (ph_index);
     BldPgmMem += prog_hdr->p_memsz;
     if (prog_hdr->p_type == PT_LOAD && Logical_Load == 0xFF0000FF)
       Logical_Load = prog_hdr->p_vaddr;
    }
 BldPgmMem = ((BldPgmMem + 65535) & -65536) + 8192;
 pResHdr->PgmHdrMem = BldPgmMem;
/*--------------------------------------------------------------------*
 *  Setup various addresses within the Payload section                *
 *--------------------------------------------------------------------*/
 pBldPay = PTROPR(char *,pResHdr,+,BldHdrStg);
 pBldFun = (ENV_DsnkFunc *) pBldPay;
 pBldSrc = PTROPR(long *,pBldFun,+,BldFunStg);
 pBldLin = PTROPR(char *,pBldSrc,+,BldSrcStg);
 pBldStr = PTROPR(char *,pBldLin,+,BldLinStg);
/*--------------------------------------------------------------------*
 *  Build the Function Element Array                                  *
 *--------------------------------------------------------------------*/
 pResHdr->OffFunc = PTROPR(long,pBldFun,-,pResHdr);  /* Set Offset    */
 pBldCurFun = pBldFun;
 for (i = 0; i < FndFunLoc; i++)
   {
    pFunLoc = SoffFunLoc[i];
    if (pFunLoc->CntSline < 1) continue;
    pBldCurFun->Name  = PTROPR(long,pFunLoc->pFSnkName,-,pStrTbl);
    if (pFunLoc->pCSnkName != NULL) pBldCurFun->Class = PTROPR(long,pFunLoc->pCSnkName,-,pStrTbl);
    pBldCurFun->Soff  = pFunLoc->Soff - Logical_Load;
    pBldCurFun->Eoff  = pFunLoc->Eoff - Logical_Load;
    pBldCurFun->FLidx = pFunLoc->FLidx;
    pBldCurFun->LLidx = pFunLoc->LLidx;
    pBldCurFun->UFidx = pFunLoc->pSrcLoc->SrcIdx;
    pBldCurFun++;
   }
/*--------------------------------------------------------------------*
 *  Build the Unique Source File Array                                *
 *--------------------------------------------------------------------*/
 pResHdr->OffUsrc = PTROPR(long,pBldSrc,-,pResHdr);
 for (pBldCurSrc = pBldSrc, pSrcLoc = pSrc1st;
      pSrcLoc != NULL;
      pBldCurSrc++, pSrcLoc = pSrcLoc->pNext)
  *pBldCurSrc = PTROPR(long,pSrcLoc->pMSnkName,-,pStrTbl);
/*--------------------------------------------------------------------*
 *  Copy the Linenumber array to the payload area.                    *
 *--------------------------------------------------------------------*/
 pResHdr->OffLine = PTROPR(long,pBldLin,-,pResHdr);
 memcpy(pBldLin,&DBGLines[0],BldLinStg);
/*--------------------------------------------------------------------*
 *  Copy the String Table to the Payload Area                         *
 *--------------------------------------------------------------------*/
 pResHdr->OffStbl = PTROPR(long,pBldStr,-,pResHdr);
 memcpy(pBldStr,pStrTbl,BldStrStg);

 return;
}

/*--------------------------------------------------------------------*
 *  Function: build_sec_list                                          *
 *  Build STAB and SYMTAB Control structures                          *
 *--------------------------------------------------------------------*/
void build_sec_list(Elf32_Word       sec_num,
                    const char      *sec_name,
                    const char      *str_name,
                    sec_list        *pSecLst)
{
 Elf32_Shdr   *pSecHdr;
 const char   *pStrName;
 Elf32_Word    j;

 pSecHdr = SEC_PTR(sec_num);
 pSecLst->Stb_Hdr = pSecHdr;
 pSecLst->Stb_Nam = sec_name;
 pSecLst->Stb_Num = sec_num;

 /* Traverse sections until we get to string section str_name */
 for (j = 0; j < ehdr.e_shnum; j++)
   {
    pStrName = NULL;
    pStrName = get_section_name(j);
    if (strcmp(pStrName,str_name) == 0)
      {
       pSecLst->Str_Hdr = SEC_PTR(j);
       pSecLst->Str_Num = j;
       pSecLst->Str_Nam = pStrName;
	 //  printf("\nSec str name = %s",pSecLst->Str_Nam);
      }
   }
 if (pSecLst->Str_Nam != NULL) pSecLst->Sec_Typ = 1;
 return;
}


/*--------------------------------------------------------------------*
 *  Function: elf_string                                              *
 *  Convert an ELF string into a real string.                         *
 *--------------------------------------------------------------------*/
const char * elf_string(Elf32_Word sec_index,
                        Elf32_Word byte_off,
                        Elf32_Word byte_rel)
{
 Elf32_Shdr *sec_hdr = SEC_PTR(sec_index);

 return((char *)(file_bytes + sec_hdr->sh_offset + byte_off + byte_rel));
}


/*--------------------------------------------------------------------*
 *  Function: get_section_name                                        *
 *  Return the Namne of an ELF section                                *
 *--------------------------------------------------------------------*/
const char * get_section_name(Elf32_Word sec_index)
{
 Elf32_Shdr *sec_hdr;

 if (sec_index == 0) return "<dummy>";
 sec_hdr = SEC_PTR(sec_index);
 return(elf_string((Elf32_Word)ehdr.e_shstrndx,sec_hdr->sh_name,0));
}

/*--------------------------------------------------------------------*
 *  Function: crc32                                                   *
 *  Generate a 32 bit CRC for an input string                         *
 *--------------------------------------------------------------------*/
static unsigned long crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
   };
unsigned long crc32(const unsigned char *s, unsigned int len)
{
 unsigned int i;
 unsigned long crc32val;

 crc32val = 0;
 for (i = 0;  i < len;  i ++)
   {
    //fprintf(stderr, "crc32val: %lu, s[%d]: 0x%x\n", crc32val, s[i]);
    crc32val = crc32_tab[(crc32val^s[i])& 0xff]^(crc32val >> 8);
   }
 return crc32val;
}

/*--------------------------------------------------------------------*
 *  Function: CmpSoff
 *  Call back function used by qsort(3) for making Soff comparison
 *--------------------------------------------------------------------*/
int CmpSoff(const void *a,const void *b)
{
   if((**(Func_Loc **)a).Soff < (**(Func_Loc **)b).Soff)
      return -1;     // First item is smaller than second one
   if ((**(Func_Loc **)a).Soff > (**(Func_Loc **)b).Soff)
      return 1;      // First item is bigger than second one
   return 0;         // They're equal
}

/*--------------------------------------------------------------------*
 *  Function: CmpName
 *  Call back function used by qsort(3) for making SName comparison
 *--------------------------------------------------------------------*/
int CmpName(const void *a, const void *b)
{
   short rc = 0;

   rc = memcmp((**(Func_Loc **)a).SName, (**(Func_Loc **)b).SName, 164);
   return (rc);
}

/*--------------------------------------------------------------------*
 *  Function: CmpCRC32
 *  Call back function used by qsort(3) for making SymCRC32 comparison
 *--------------------------------------------------------------------*/
int CmpCRC32(const void *a, const void *b)
{
   if((**(Func_Loc **)a).SymCRC32 < (**(Func_Loc **)b).SymCRC32)
      return -1;     // First item is smaller than second one
   if ((**(Func_Loc **)a).SymCRC32 > (**(Func_Loc **)b).SymCRC32)
      return 1;      // First item is bigger than second one
   return 0;         // They're equal
}
/*--------------------------------------------------------------------*
 *  Function: CmpLineNum
 *  Call back function used by qsort(4) for making line number comparison
 *--------------------------------------------------------------------*/

int CmpLineNum(const void *a, const void *b)
{
	if((**(Func_Loc **)a).FLNum < (**(Func_Loc **)b).FLNum)
		return -1;
	if((**(Func_Loc **)a).FLNum > (**(Func_Loc **)b).FLNum)
		return 1;
    return 0;
}
/*--------------------------------------------------------------------*
 *  Force the following functions to be generated as inline           *
 *  instructions rather than function calls                           *
 *--------------------------------------------------------------------*/
#pragma intrinsic(memcpy,memset,strcmp,strlen)
/*--------------------------------------------------------------------*
 * FUNCTION NAME = EnvSinkInit                                        *
 *                                                                    *
 * DESCRIPTIVE NAME = DLL Map Sink Control initialization             *
 *                                                                    *
 * FUNCTION = Perform setup processing to allow DLL Map               *
 *            String Sink to be used within current Process           *
 *                                                                    *
 * INPUTS: None                                                       *
 *                                                                    *
 * OUTPUTS: Return Code that describes result of operation            *
 *                                                                    *
 *--------------------------------------------------------------------*
 * NOTES :                                                            *
 *    Acquires storage and creates data structures required to        *
 *    implement a String Sink.                                        *
 *--------------------------------------------------------------------*
 * CHANGE ACTIVITY :                                                  *
 *                                                                    *
 *--------------------------------------------------------------------*/
short      ENV_DSnkInit(void)
 {
   void                    *pVsa;       /* Initial Alloc storage area */
   ENV_DllSinkBlk          *pDsb;       /* DLLMap Sink Block          */

/*--------------------------------------------------------------------*
 *  Acquire Storage for the DLLMap Sink Control structure             *
 *  from the internal DLLMap System Heap.                             *
 *--------------------------------------------------------------------*/
 if ((pDsc = (ENV_DllSinkCtl *) calloc(1,sizeof(ENV_DllSinkCtl))) == NULL) goto InitCMerr;
/*--------------------------------------------------------------------*
 *  Acquire Win32 Storage Region for DLL Sink areas.                  *
 *--------------------------------------------------------------------*/
   if ((pVsa  =  calloc(1,ENV_STR_SNK)) == NULL) goto InitCMerr;
   if (pVsa == NULL) goto InitCMerr;
/*--------------------------------------------------------------------*
 *  Initialize DLLMap Sink areas                                      *
 *--------------------------------------------------------------------*/
   pDsb = (ENV_DllSinkBlk *) pVsa;     /* Cast Block Reference        */
   pDsc->pVOSink = (char *) &pDsb->Sink;             /* 1st Sink Byte */
   pDsc->pCOSink = (char *) &pDsb->Sink;             /* Cur Sink Byte */
   pDsc->pList = (char *) &pDsb->List;               /* List Array    */
   pDsc->pEOSink = PTROPR(char *,pDsc->pList,-,1);
   pDsc->Size = ENV_STR_LST;           /* initial list size           */
   pDsc->CMax = ENV_STR_LST/sizeof(ENV_DllListEle);
   pDsc->EFree = pDsc->CMax-1;
   return(PST_SUCCESS);

/*--------------------------------------------------------------------*
 *  Standard Initialization Failed exit                               *
 *--------------------------------------------------------------------*/
 InitCMerr:
    return(PST_FAILED);                /* Return error code to caller */
 }

/*--------------------------------------------------------------------*
 * FUNCTION NAME = ENV_DSnkString                                     *
 *                                                                    *
 * DESCRIPTIVE NAME = DLL Map String Sink Add element                 *
 *                                                                    *
 * FUNCTION = Evaluate an input text string value. If value is not    *
 *            in the Sink add it, return the pointer to the newly     *
 *            added string value or the ptr to an existing instance.  *
 *                                                                    *
 * INPUTS:    char    Zero-terminated text string to add              *
 *            char ** Pointer to Pointer to hold returned Sink        *
 *                    Text value pointer                              *
 *                                                                    *
 * OUTPUTS: Return Code that describes result of operation            *
 *          Indirect outputs include pointer to a string instance     *
 *          contained in the sink, and updated String List control    *
 *          information.                                              *
 *--------------------------------------------------------------------*
 * NOTES :                                                            *
 *        The input text value is used as a lookup key in the         *
 *        String List to determine if an element already exists.      *
 *        If an element is found its text pointer is returned to the  *
 *        caller. The element use count is incremented as well.       *
 *                                                                    *
 *        If no element is found space is acquired to hold the text   *
 *        in the Sink Block. If no space is available an error        *
 *        is returned.                                                *
 *                                                                    *
 *        The insertion point in the Sink List is determined. List    *
 *        elements are stored in ascending collating text sequence.   *
 *        A new element is inserted, the text value pointer saved     *
 *        and the use count set to 1.                                 *
 *--------------------------------------------------------------------*
 *--------------------------------------------------------------------*/
short      ENV_DSnkString   (char *Txtval,
                             char **ppSinkval)
 {
   ENV_DllListEle          *pSse;       /* String Sink List element   */
   ENV_DllListEle          *pSs2;       /* String Sink List element   */
   void                    *pMoveT;     /* List target address        */
   void                    *pMoveS;     /* List source address        */
   char                    *pSnkStr;    /* Sink String address        */
   unsigned long            MoveL;      /* Move List length           */
   long                     FndEle;     /* List subscript value       */
   short                    TLen;       /* Length of Sink value       */
   short                    MyRet;      /* Local Return Code          */

   *ppSinkval = NULL;                  /* Init the return pointer     */
/*--------------------------------------------------------------------*
 *  Call DSnkLocate to determine if the input Text string is already  *
 *  contained in the Sink. The List insert or active element for the  *
 *  string isa returned.                                              *
 *--------------------------------------------------------------------*/
   if ((MyRet = ENV_DSnkLocate(Txtval,&FndEle,ppSinkval,&pSse)) == PST_SUCCESS)
/*--------------------------------------------------------------------*
 *  Match found. pSse = Ptr to SinkListEle                            *
 *    Increment Use count to reflect additional access                *
 *--------------------------------------------------------------------*/
     ++pSse->UseCnt;                   /* Account for new reference   */
/*--------------------------------------------------------------------*
 *  Not found. pSse = Ptr to List insertion point.                    *
 *    Allocate Sink Block Storage for Text element                    *
 *    Create new List element                                         *
 *    Insert element into proper list location                        *
 *    If List is full and no garbage return an error                  *
 *--------------------------------------------------------------------*/
   else
     {
      TLen = strlen(Txtval)+1;
      if ((pDsc->pCOSink + TLen) > pDsc->pEOSink) goto ServNOSPCerr;
/*--------------------------------------------------------------------*
 *  Return Pointer is Current Sink Byte. New Current = Current + Len  *
 *--------------------------------------------------------------------*/
      pSnkStr = pDsc->pCOSink;
      pDsc->pCOSink = PTROPR(char *,         /* Cast Type             */
                             pDsc->pCOSink,  /* Current Sink Used     */
                             +,              /* Add operation         */
                             TLen);          /* Input Area size       */
/*--------------------------------------------------------------------*
 *  Move text to proper slot                                          *
 *--------------------------------------------------------------------*/
      memcpy(pSnkStr,Txtval,TLen);     /* Copy the input string       */
      pDsc->BlkUsed += TLen;           /* Update used space size      */
      *ppSinkval = pSnkStr;            /* Set for caller              */
/*--------------------------------------------------------------------*
 *  *--------------------------------------------------------------*  *
 *  * Insert an entry in the String list                           *  *
 *  *--------------------------------------------------------------*  *
 *--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*
 *  Pre-Check - Determine if List has Free elements                   *
 *--------------------------------------------------------------------*/
     if (pDsc->CMax == pDsc->CUsed) goto ServLISTFULLerr;
/*--------------------------------------------------------------------*
 *  Insertion CASE 1 - The list is empty.                             *
 *  Claim Free element and add new element as 1st in list             *
 *--------------------------------------------------------------------*/
     if (pDsc->CActive < 1)
       {
        pSse->pSText = pSnkStr;
        pSse->UseCnt = 1;
        pDsc->EStart = FndEle;
        pDsc->EEnd = FndEle;
        --pDsc->EFree;
        ++pDsc->CUsed;
        pDsc->CActive = 1;
       }
/*--------------------------------------------------------------------*
 *  Insertion CASE 2 - Insertion point is 1st list element            *
 *  Claim Free element and add new element as 1st in list             *
 *--------------------------------------------------------------------*/
     else if (FndEle == pDsc->EStart)
       {
        pSse = PTROPR(ENV_DllListEle *,pSse,-,sizeof(ENV_DllListEle));
        pSse->pSText = pSnkStr;
        pSse->UseCnt = 1;
        --pDsc->EFree;
        --pDsc->EStart;
        ++pDsc->CUsed;
        ++pDsc->CActive;
       }
/*--------------------------------------------------------------------*
 *  Insertion CASE 3 - Insertion point is internal to list            *
 *  Compute a move sequence to slide list left 1 element              *
 *--------------------------------------------------------------------*/
     else
       {
/*--------------------------------------------------------------------*
 *  Compute move length and target address                            *
 *--------------------------------------------------------------------*/
        MoveL = (FndEle-pDsc->EStart)*sizeof(ENV_DllListEle);
        pMoveT  = PTROPR(void *,
                         pDsc->pList,
                         +,
                        (pDsc->EFree*sizeof(ENV_DllListEle)));
        pMoveS  = PTROPR(void *,
                         pDsc->pList,
                         +,
                        (pDsc->EStart*sizeof(ENV_DllListEle)));
        memcpy(pMoveT,pMoveS,MoveL);   /* Open up a free slot         */
        pSse = --pSse;
        pSse->pSText = pSnkStr;
        pSse->UseCnt = 1;
        --pDsc->EFree;
        --pDsc->EStart;
        ++pDsc->CUsed;
        ++pDsc->CActive;
       }
     }

   *ppSinkval = pSse->pSText;          /* Set Sink Text val Ptr       */
   return(PST_SUCCESS);

 ServNOSPCerr:
   return(ENVERR_DSNKNOSPACE);

 ServLISTFULLerr:
   return(ENVERR_DSNKLISTFULL);

 ServCMerr:
   return(PST_FAILED);
 }

/*--------------------------------------------------------------------*
 * FUNCTION NAME = ENV_DsnkLocate                                     *
 *                                                                    *
 * DESCRIPTIVE NAME = DLL Map locate text value in Sink               *
 *                                                                    *
 * FUNCTION = Attempt to locate the Sink Text value that matches      *
 *            the input text string. If found return the Sink Value   *
 *            pointer. If not found return an error.                  *
 *                                                                    *
 * INPUTS:    char    Zero-terminated text string to verify           *
 *            long  * Variable to return Found Index subscript        *
 *            char ** Pointer to Pointer to hold returned Sink        *
 *                    Text value pointer                              *
 *            char ** List insert or active element for string        *
 *                                                                    *
 * OUTPUTS: Return code that describes result of operation            *
 *                                                                    *
 *--------------------------------------------------------------------*
 * NOTES :                                                            *
 *     1. The input text value pointer is used as a search key in the *
 *        String List. If a matching element is found it text value   *
 *        pointer is returned to the caller. If no match exists       *
 *        an error response is generated.                             *
 *        The Sink List element use count is not incremented, nor     *
 *        is any attempt made to add a new element.                   *
 *     2. When DSnkLocate is called by DSnkString to locate           *
 *        an entry in the List the address of the List entry to       *
 *        to insert after is returned as well as the Element number.  *
 *--------------------------------------------------------------------*
 *--------------------------------------------------------------------*/
short      ENV_DSnkLocate   (char            *Txtval,
                             long            *FndEle,
                             char           **ppSinkval,
                             ENV_DllListEle **ppInsEle)
 {
   char                    *pLst;       /* String Sink Element array  */
   ENV_DllListEle          *pSse=NULL;  /* String Sink Element        */
   short                    i=0;        /* Compare control            */
   long                     SrcEle;     /* Search Element subscript   */
   long                     LoEle;      /* BinSearch Control          */
   long                     HiEle;      /* BinSearch Control          */

   if (pDsc->CActive < 1) goto ListEMPerr; /* List is empty           */
/*--------------------------------------------------------------------*
 *  If the List contains a single element evaluate it directly        *
 *--------------------------------------------------------------------*/
   SrcEle = pDsc->EStart;
   pLst = pDsc->pList;
   if (pDsc->CActive == 1)
     {
      pSse = PTROPR(ENV_DllListEle *,
                    pLst,
                    +,
                    (SrcEle*sizeof(ENV_DllListEle)));
      if (( i = strcmp(Txtval,pSse->pSText)) == 0)
       goto ListFNDele;
      else  goto ListSRCerr;
     }
/*--------------------------------------------------------------------*
 *  For multiple elements perform a Binary Search                     *
 *--------------------------------------------------------------------*/
   else
     {
      LoEle = pDsc->EStart;                  /* Array low Element     */
      HiEle = pDsc->EEnd;                    /* Array high element    */
      while (LoEle <= HiEle)
        {
         SrcEle = (LoEle + HiEle)/2;        /* Mid-point of scan     */
         pSse = PTROPR(ENV_DllListEle *,
                       pLst,
                       +,
                      (SrcEle*sizeof(ENV_DllListEle)));
         if ((i = strcmp(Txtval,pSse->pSText)) == 0)
             goto ListFNDele;
         if (i < 0) HiEle = SrcEle-1;
         else LoEle = SrcEle+1;
        }
      goto ListSRCerr;
     }
/*--------------------------------------------------------------------*
 *  An Array element for the input text string was found set its Text *
 *  value pointer.                                                    *
 *  Return the SinkList Element address for an internal request       *
 *--------------------------------------------------------------------*/
 ListFNDele:
   if (FndEle) *FndEle = SrcEle;     /* Return subscript value        */
   *ppSinkval = pSse->pSText;        /* Set Sink Text val Ptr         */
   if (ppInsEle) *ppInsEle = pSse;   /* Set ElePtr                    */
   return(PST_SUCCESS);
/*--------------------------------------------------------------------*
 *  List is empty point to 1st free slot                              *
 *--------------------------------------------------------------------*/
 ListEMPerr:
   SrcEle = pDsc->EFree;
   pSse = PTROPR(ENV_DllListEle *,
                 pDsc->pList,
                 +,
                (SrcEle*sizeof(ENV_DllListEle)));
   i = 0;

/*--------------------------------------------------------------------*
 *  String Greater than ListEle adjust to point to next element       *
 *--------------------------------------------------------------------*/
 ListSRCerr:
   if (i > 0)
     {
      pSse = PTROPR(ENV_DllListEle *,
                    pSse,
                    +,
                    sizeof(ENV_DllListEle));
      ++SrcEle;                      /* increment element index    */
     }
   if (FndEle) *FndEle = SrcEle;     /* Return subscript value        */
   if (ppInsEle) *ppInsEle = pSse;   /* Set ElePtr                    */
   return(ENVERR_DSNKNFND);
 }


 // Dwarf related functions
 /*--------------------------------------------------------------------*
 *  Function: build_sfl_dwarf		                                   *
 *	This function gets each cu header from the binary file, gets the   *
 *  first die in the current CU(Compilation Unit)  and calls           *
 *	get_die_and_siblings function for each of them. 				   *
 *---------------------------------------------------------------------*/

 void build_sfl_dwarf(Dwarf_Debug dbg,Elf32_Shdr *sec_hdr)
{
	Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Error error;
    int cu_number = 0;
    for(;;++cu_number) {
        struct srcfilesdata sf;
        sf.srcfilesres = DW_DLV_ERROR;
        sf.srcfiles = 0;
        sf.srcfilescount = 0;
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        int res = DW_DLV_ERROR;

		/* dwarf_next_cu_header - Returns offset of next compilation-unit thru next_cu_offset
        pointer.     It basically sequentially moves from one  cu to the next.  The current 
		cu is recorded internally by libdwarf.*/

        res = dwarf_next_cu_header(dbg,&cu_header_length,
            &version_stamp, &abbrev_offset, &address_size,
            &next_cu_header, &error);
        if(res == DW_DLV_ERROR) {
            printf("Error in dwarf_next_cu_header\n");
            exit(1);
        }
        if(res == DW_DLV_NO_ENTRY) {
            /* Done. */
            return;
        }
        /* The CU will have a single sibling, a cu_die. 
		Given a Dwarf_Debug dbg, and a Dwarf_Die die, it returns 
		a Dwarf_Die for the sibling of die.  In case die is NULL, 
		it returns (thru ptr) a Dwarf_Die for the first die in the current 
		cu in dbg.  Returns DW_DLV_ERROR on error.                         */

        res = dwarf_siblingof(dbg,no_die,&cu_die,&error);
        if(res == DW_DLV_ERROR) {
            printf("Error in dwarf_siblingof on CU die \n");
            exit(1);
        }
        if(res == DW_DLV_NO_ENTRY) {
            /* Impossible case. */
            printf("no entry! in dwarf_siblingof on CU die \n");
            exit(1);
        }
        get_die_and_siblings(dbg,cu_die,0,&sf);
        dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
        resetsrcfiles(dbg,&sf);
	//	printf("\nNo of cus = %d",cu_number);
    }
}
 /*------------------------------------------------------------------------*
 *  Function: get_die_and_siblings		                                   *
 *  This function recursively gets all the dies and process the die data.  *
 *-------------------------------------------------------------------------*/

void get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,int in_level,
   struct srcfilesdata *sf)
{
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die=in_die;
    Dwarf_Die child = 0;
    Dwarf_Error error;
    

    process_die_data(dbg,in_die,sf);

    for(;;) {
        Dwarf_Die sib_die = 0;
        res = dwarf_child(cur_die,&child,&error);
        if(res == DW_DLV_ERROR) {
            printf("Error in dwarf_child , level %d \n",in_level);
            exit(1);
        }
        if(res == DW_DLV_OK) {
            get_die_and_siblings(dbg,child,in_level+1,sf);
        }
        /* res == DW_DLV_NO_ENTRY */
        res = dwarf_siblingof(dbg,cur_die,&sib_die,&error);
        if(res == DW_DLV_ERROR) {
            printf("Error in dwarf_siblingof , level %d \n",in_level);
            exit(1);
        }
        if(res == DW_DLV_NO_ENTRY) {
            /* Done at this level. */
            break;
        }
        /* res == DW_DLV_OK */
        if(cur_die != in_die) {
            dwarf_dealloc(dbg,cur_die,DW_DLA_DIE);
        }
        cur_die = sib_die;
        process_die_data(dbg,cur_die,sf);
    }
    return;
}
 /*--------------------------------------------------------------------*
 *  Function: process_die_data		                                   *
 *  This function would get all the tags and do processing as per the  *
 *  tag	            							   *
 *--------------------------------------------------------------------*/

void process_die_data(Dwarf_Debug dbg, Dwarf_Die print_me,struct srcfilesdata *sf)
{
    char *name = 0;
	char *linkname = 0;
	char *cuname = 0;
    Dwarf_Error error = 0;
    Dwarf_Half tag = 0;
    const char *tagname = 0;
    char* filename;
	char *basefilename;
    Dwarf_Signed sectionCount = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    char         StrText[160];
	short        Have_Func_Entry=0;

//SFL Related
	short LinkNameGot;

    pSrcLoc = NULL;
    pSrcPrv = NULL;
	pFunLoc = NULL;

    int res = dwarf_diename(print_me,&name,&error);

    if(res == DW_DLV_ERROR) {
        printf("Error in dwarf_diename , \n");
        exit(1);
    }
    if(res == DW_DLV_NO_ENTRY) {
	   name = "<no DW_AT_name attr>";
    }
    res = dwarf_tag(print_me,&tag,&error);
    if(res != DW_DLV_OK) {
        printf("Error in dwarf_tag \n");
        exit(1);
    }
   // res = dwarf_get_TAG_name(tag,&tagname);
//	printf("\tTag Name = %s",tagname);

    if(res != DW_DLV_OK) {
        printf("Error in dwarf_get_TAG_name \n");
        exit(1);
    }
	if(tag == DW_TAG_compile_unit){
			CU_die = print_me; // store the CU Die, this is required for finding the line numbers.
			int res = dwarf_diename(CU_die,&cuname,&error);
			strcpy(StrText,cuname); // for using later
            pStrText = StrText; //basename(StrText);

	}
	if(tag == DW_TAG_inlined_subroutine){  // For handling inline functions. This is not done now. 

	}

	if( tag == DW_TAG_subprogram) {
           int res = dwarf_diename(print_me,&name,&error);

			/* ------------------------------------------------------------------------------ *
			 *  Get lowpc and highpc for this subprogram. If it has a valid low and high pc , * 
			 *  then we get the line numbers for this function in the source file . Also we   *
			 *  get the HP Linkage name(mangled name) because CRC for the functions in Symbol *
			 *  table is calculated using mangled name. This linkage name will be compared    *
			 *  with the functions in the Sumbol table for a match in CRC                     *
			 * ------------------------------------------------------------------------------ */
			lowpc = highpc = 0;
			get_highlowpc_linkagename(dbg,print_me,sf,&lowpc,&highpc,&linkname,&LinkNameGot);


			/*-------------------------------------------------------------------------------*
			 * Function to process all the details about the sub program. All the information*
			 * line function name, line details are all populated here and added to the      *
			 * source file                                                                   *
			 *-------------------------------------------------------------------------------*/
			int processed = process_subprogram(dbg,lowpc,highpc,&linkname,LinkNameGot,&name,sectionCount); 
			if(processed == 0) // no need to process
			 return;
					
        }
		else if (tag == DW_TAG_compile_unit ||
           tag == DW_TAG_partial_unit ||
           tag == DW_TAG_type_unit) {
            resetsrcfiles(dbg,sf);
         //   printf(    " source file           : %s\n",name);
           // print_comp_dir(dbg,print_me,level,sf);
        }

    //dwarf_dealloc(dbg,name,DW_DLA_STRING);
    dwarf_dealloc(dbg,linkname,DW_DLA_STRING);
    dwarf_dealloc(dbg,cuname,DW_DLA_STRING);
  /*  dwarf_dealloc(dbg,tagname,DW_DLA_STRING);
    dwarf_dealloc(dbg,filename,DW_DLA_STRING);
    dwarf_dealloc(dbg,basefilename,DW_DLA_STRING);*/
}

int process_subprogram(Dwarf_Debug dbg,Dwarf_Addr lowpc1,Dwarf_Addr highpc1,
                        char **HPLinkagename,short LinkNameGot,char **name,Dwarf_Signed sectionCount)
{
    unsigned int SymDescLen;
    char         StrText[160];
    unsigned long SymCRC32;
    Dwarf_Line *linebuf = NULL;
	Dwarf_Signed linecount = 0;
	Dwarf_Addr retaddr;
	Dwarf_Addr lastline_addr;
	Dwarf_Addr firstline_addr;
	Dwarf_Addr prev_addr;
	char buffer[10];
	char buffer1[10];
    char* filename;
	char *basefilename;
    int function_in_file = 0;
	Dwarf_Unsigned last_lno = 0;
	Dwarf_Unsigned prev_lno =0;
	Dwarf_Unsigned first_lno = 0;
	Func_Loc    *pFun = NULL;
    Dwarf_Error error = 0;
	short        Have_Func_Entry=0;

    int in=0;
	int sectionIndex = 0;
    int iCounter = 0;
	int counter = 0;
	int len = 0;
	int k=0;

	if(lowpc1 && highpc1) {
		pFunLoc = NULL;
		/*--------------------------------------------------------------------*
		 * Use the mangled name if availableto check the CRC match in the     *
		 * CRC array                                                          *
		 *--------------------------------------------------------------------*/
		if(LinkNameGot){ 

		   SymDescLen = strlen(*HPLinkagename);
		   memcpy(StrText,*HPLinkagename,SymDescLen);
		}
		else{ // use the actual function name
			SymDescLen = strlen(*name);
			memcpy(StrText,*name,SymDescLen);

		}
	/*------------------------------------------------------------------------*
     * Find out the function in the CRC array with a matching CRC value       *
	 *------------------------------------------------------------------------*/
		StrText[SymDescLen] = 0x00;

		SymCRC32 = crc32((const unsigned char *) &StrText,SymDescLen);
		pFunLoc = check_crc_match(SymCRC32);

	}
	/*-------------------------------------------------------------------------*
	 * If function name not found in CRC array, no need to process further     *
	 * Also if there is no low and high pc values no need to proceed           *
	 *-------------------------------------------------------------------------*/
	if (pFunLoc == NULL)   
		return 0;

	/*--------------------------------------------------------------------*
	 *  Determine if entry was found                                      *
	 *--------------------------------------------------------------------*/
     if (pFunLoc->FuncRef++ == 0)
        Have_Func_Entry = 1;
     pFunLoc->FuncType = pFunLoc->FuncRef > 1 ? 'D' : 'F';
     if (pFunLoc->FuncRef > 1)  
		return 0;

    /*-------------------------------------------------------------------------------------------------------*
	 * Get the details of line number from debug_line section and  populate the structure. For each function * 
	 * which is valid(ie having low pc and high pc) get the first line number from lowpc. Finding the last   *
	 * line number is not very straight forward. Each CU DIE can have many sets and inside each set the pc   *
	 * address increases. We mostly get correct line number for the first line. But for the last line we need* 
	 * to check each lines after the first line number and check whether this pc address is below the high pc*
	 * address. we may not get an extact match for last line number with the high pc value we got. Also we   *
	 * need to check that the pc address belong to the same set. This means the first line number and last   *
	 * line number should belong to the same set. Also store the file name to which this line  number belongs* 
	 * to. If this file is different from the current file name, then we need to create entries for this file*
	 * and its functions also in the src list                                                                *
	 *-------------------------------------------------------------------------------------------------------*/
	 int lres = dwarf_srclines(CU_die, &linebuf, &linecount, &error); // line buffer for this CU
	 //printf("\n File name = %s : ", pStrText);
 	// printf( "  subprogram            : %s\n",*name);
	// printf("\nFun name got from CRC is %s",pFunLoc->SName);
	// printf("\nLine count = %d", linecount);
	// printf("\nLow pc = %x", lowpc1);
	 for(in=0;in<linecount;in++)
	 {
		lres = dwarf_lineaddr(linebuf[in],&retaddr,&error); // retaddr for the current line
		if(lres == DW_DLV_ERROR) 
		  printf("Error in dwarf_lineaddr \n");

		/*-----------------------------------------------------------------------------------------*
		 * Get the first line number details                                                       *
		 *-----------------------------------------------------------------------------------------*/
		if(lowpc1 && (retaddr == lowpc1))
		{
			/*-------------------------------------------------------------------------------------------------*
			 * Get the filename from the line buffer and store this. Sometimes the filename in the line buffer *
			 * differs from the one actually got before. In this case we are storing the info related to this  *
			 * name got from line buffer. If both the names are same also, we store the information from line  *
			 * buffer.  		                                                                               *
			 *-------------------------------------------------------------------------------------------------*/
			int sres = dwarf_linesrc(linebuf[in], &filename, &error); 
			basefilename = strrchr(filename,'/');
			convert_to_upper(basefilename+1);
			pFileName =  basefilename+1;  // Absolute File name in CAPS

			// Check whether this function is already added in the file pFileName(pFunLoc is global)
			// After this function we get the pSrcLoc also.
			function_in_file = search_function_in_file(&pFileName);

			if(function_in_file) // this function already in file, so don't process further
				return 0;

		   	lres = dwarf_lineno(linebuf[in],&first_lno,&error);
			firstline_addr = retaddr;

		
			if(lres == DW_DLV_ERROR) 
				 printf("Error in dwarf_lineno \n");

			// if first line number is got, find the set  to which this belongs to .
			/*for(counter =0;counter < sectionCount; counter++)
			{
				sprintf(buffer,"%"DW_PR_DUx,(Dwarf_Unsigned)setadds[counter]);
				sprintf(buffer1,"%x",retaddr);
				if(strstr(buffer,buffer1) != NULL){ // match found for the set 
					sectionIndex = counter+1; 
					break;
				}
			}*/

			//Store the first line number details in SFL structure
		   // pFunLoc->CntSline++; 
			if (pFunLoc->FLOff == 0)
			{
			   pFunLoc->FuncType = 'F';
               SrcForFun++;
		       pFunLoc->FLidx = CntSrcLine;
			   pFunLoc->FLidxStab = 0;//stab_num;
			   //pFunLoc->FLOff = firstline_addr;
			   pFunLoc->FLOff = firstline_addr - pFunLoc->Soff;

               pFunLoc->FLNum = first_lno;
               len = strlen(pFileName);
		       len = len > 27 ? 27 : len;
		       if (pSrcLoc == NULL)
				{
                  pSrcLoc = (SrcFile_Loc *) malloc(sizeof(SrcFile_Loc));
		          memset((void *) pSrcLoc,0x00,sizeof(SrcFile_Loc));
				  pSrcLoc->StabSOIdx = 0;//SoLNum;
                  pSrcLoc->pName = pFileName;
//		          pSrcLoc->StabSOIdx = SoLNum;
				  if (pSrc1st == NULL) pSrc1st = pSrcLoc;
	              else pSrcPrv->pNext = pSrcLoc;
		          pSrcLoc->SrcIdx = UseSrcLoc++;
				  memcpy(pSrcLoc->MemName,pFileName,len);
	            }
				if (pSrcLoc->pFunHoC == NULL) pSrcLoc->pFunHoC = pFunLoc;

				pFunLoc->pSrcLoc = pSrcLoc;
				/*--------------------------------------------------------------------------------------------*
				 * The order of functions got from dwarf reader is not the same as the order in which it is   *
				 * defined in the source file. Hence the functions are sorted based on the first line numbers *
				 * of the function. If file name in the line buffer is different from the name got from the   *
				 * DWARF , we cannot add directly to this array as this array contains the functions for the  *
				 * original file(file name got from DWARF). so instead of directly adding to this array for   *
				 * sorting, take the functions from the source file(pSrcLoc) and then add to this array. Then *
				 * sort this array  we will get the correct order											  *
				 *--------------------------------------------------------------------------------------------*/

				// getting the functions in this file(pSrcLoc)
				for(pFun = pSrcLoc->pFunHoC ,iCounter=0;iCounter < pSrcLoc->RefCnt ;pFun = pFun->pSrcFun,iCounter++)
				{
					SortedFunLoc[iCounter] = pFun;
				}
				SortedFunLoc[iCounter] = pFunLoc; // Now add to the last 
				pSrcLoc->RefCnt++;
			    qsort((void *)SortedFunLoc, pSrcLoc->RefCnt, sizeof(struct s_func_loc *), CmpLineNum);
				
   				// Storing the next function name for each function in the source file from the sorted array
				// The sorted array is used here to get all the consecutive functions to be stored. 
				for(k=0;k<pSrcLoc->RefCnt-1;k++){
					pSrcLoc->pFunHoC = SortedFunLoc[k];
					pSrcLoc->pFunHoC->pSrcFun = SortedFunLoc[k+1];
				//	printf("\nStoring fn name = %s", pSrcLoc->pFunHoC->SName);
				}
						
				// storing the first and last function name.
				pSrcLoc->pFunHoC = SortedFunLoc[0];
				pSrcLoc->pFunToC = SortedFunLoc[pSrcLoc->RefCnt-1];
		
			}
					
			prev_lno = last_lno;
			prev_addr = firstline_addr;
			
		}
		/*---------------------------------------------------------------------------------------*
		 * If first line number is got, then also start storing the next line numbers and check  *
         * whether this is the last line number                                                  *
		 *---------------------------------------------------------------------------------------*/
		if(first_lno){ 
			pFunLoc->CntSline++; 
			lres = dwarf_lineno(linebuf[in],&last_lno,&error);
			lastline_addr = retaddr; 
			if(lres == DW_DLV_ERROR) 
			  printf("Error in dwarf_lineno \n");
			// check only if sectionIndex is less than the total set counts.
			// If there is only one set, no need to check.
			/*if(sectionIndex < sectionCount) 
			{
				sprintf(buffer,"%"DW_PR_DUx,(Dwarf_Unsigned)setadds[sectionIndex]);
				sprintf(buffer1,"%x",retaddr);
				if(strstr(buffer,buffer1) !=NULL){ // a match found with the next set. so break
					last_lno = prev_lno;
					lastline_addr = prev_addr;
					break;
				}
			}*/
			if(retaddr >= highpc1){
				last_lno = prev_lno;
				lastline_addr = prev_addr;
				break;
			}
					
		   /*--------------------------------------------------------------------*
	        *  Record the Debugging Line Number in Global Array  for each line   *
		    *--------------------------------------------------------------------*/
  		   // DBGLines[CntSrcLine].Offset = retaddr;
		
		    DBGLines[CntSrcLine].Offset = retaddr - pFunLoc->Soff;

    		DBGLines[CntSrcLine].LineNo = last_lno;
     		CntSrcLine++;
		    prev_lno = last_lno;
		    prev_addr = retaddr;
	    } // end if(first_lno)
    } // end of for loop

	// store the information about last line number
	if(last_lno){
				//printf("\nLast line no = %d",last_lno);
				//printf("\t Address = %x\n", lastline_addr);
				
				pFunLoc->LLidx = CntSrcLine-1; 
		        //pFunLoc->LLOff = lastline_addr;
				pFunLoc->LLOff = lastline_addr - pFunLoc->Soff;

				pFunLoc->LLNum = last_lno;
				//printf("\nNo of lines = %d",pFunLoc->CntSline);

    }
	dwarf_srclines_dealloc(dbg, linebuf, linecount);

}

void get_highlowpc_linkagename(Dwarf_Debug dbg,Dwarf_Die die,
    struct srcfilesdata *sf,Dwarf_Addr *lowpc1,Dwarf_Addr *highpc1,char **HPLinkagename,short *LinkNameGot)
{
    int res;
    Dwarf_Error error = 0;
    Dwarf_Attribute *attrbuf = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Signed attrcount = 0;
    Dwarf_Unsigned i;
    Dwarf_Unsigned filenum = 0;
    Dwarf_Unsigned linenum = 0;
    Dwarf_Unsigned num = 0;

    char *filename = 0;
	Dwarf_Signed linecount = 0;
    Dwarf_Line *linebuf = NULL;
	*LinkNameGot = 0;

    res = dwarf_attrlist(die,&attrbuf,&attrcount,&error);
    if(res != DW_DLV_OK) {
        return;
    }
    for(i = 0; i < attrcount ; ++i) {
        Dwarf_Half aform;
        res = dwarf_whatattr(attrbuf[i],&aform,&error);

        if(res == DW_DLV_OK) {
            if(aform == DW_AT_decl_file) {
                  get_number(attrbuf[i],&filenum);
                  if((filenum > 0) && (sf->srcfilescount > (filenum-1))) {
                       filename = sf->srcfiles[filenum-1];
                  }
            }
            if(aform == DW_AT_decl_line) 
			{
                  get_number(attrbuf[i],&linenum);
            }
            if(aform == DW_AT_low_pc) {

                  get_addr(attrbuf[i],lowpc1);
            }
            if(aform == DW_AT_high_pc) {
				get_addr(attrbuf[i],highpc1);

            }
			if (aform == DW_AT_HP_linkage_name)
			{
				char * temps = 0;
				int sres = dwarf_formstring(attrbuf[i], &temps,&error);
				*HPLinkagename = temps;
				*LinkNameGot = 1;
			}
        }
        dwarf_dealloc(dbg,attrbuf[i],DW_DLA_ATTR);
    }
    if(filenum || linenum) {
   //     printf("file: %" DW_PR_DUu " %s  line %"
   //            DW_PR_DUu "\n",filenum,filename?filename:"",linenum);
    }

    dwarf_dealloc(dbg,attrbuf,DW_DLA_LIST);
}


void get_highlowpc_procedurename(Dwarf_Debug dbg,Dwarf_Die die,
    struct srcfilesdata *sf,Dwarf_Addr *lowpc1,Dwarf_Addr *highpc1,char *proc_name_buf, int proc_name_buf_len)
{
	  int res;
    Dwarf_Error error = 0;
    Dwarf_Attribute *attrbuf = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Signed attrcount = 0;
    Dwarf_Unsigned i;
    Dwarf_Unsigned filenum = 0;
    Dwarf_Unsigned linenum = 0;
    Dwarf_Unsigned num = 0;

    char *filename = 0;
	Dwarf_Signed linecount = 0;
    Dwarf_Line *linebuf = NULL;
	proc_name_buf[0] = 0;

    res = dwarf_attrlist(die,&attrbuf,&attrcount,&error);
    if(res != DW_DLV_OK) {
        return;
    }
    for(i = 0; i < attrcount ; ++i) {
        Dwarf_Half aform;
        res = dwarf_whatattr(attrbuf[i],&aform,&error);

       // printf("\nAttribute form = %d",aform);
        if(res == DW_DLV_OK) {
            if(aform == DW_AT_decl_file) {
                  get_number(attrbuf[i],&filenum);
                  if((filenum > 0) && (sf->srcfilescount > (filenum-1))) {
                       filename = sf->srcfiles[filenum-1];
                  }
            }
            if(aform == DW_AT_decl_line) 
			{
                  get_number(attrbuf[i],&linenum);
            }
            if(aform == DW_AT_low_pc) {
				  get_addr(attrbuf[i],lowpc1);

            }
            if(aform == DW_AT_high_pc) {
                  get_addr(attrbuf[i],highpc1);

            }
			if(aform == DW_AT_abstract_origin)
			{
			}
			if(aform == DW_AT_inline)
			{
			}
			if (aform == DW_AT_HP_linkage_name)
			{
				char * temps = 0;
				int sres = dwarf_formstring(attrbuf[i], &temps,&error);
				
			}
        }
        dwarf_dealloc(dbg,attrbuf[i],DW_DLA_ATTR);
    }
    if(filenum || linenum) {
        printf("file: %" DW_PR_DUu " %s  line %"
               DW_PR_DUu "\n",filenum,filename?filename:"",linenum);
    }
   
    dwarf_dealloc(dbg,attrbuf,DW_DLA_LIST);
}


// this function is not used. used for debugging purpose
void print_comp_dir(Dwarf_Debug dbg,Dwarf_Die die,int level, struct srcfilesdata *sf)
{
    int res;
    Dwarf_Error error = 0;
    Dwarf_Attribute *attrbuf = 0;
    Dwarf_Signed attrcount = 0;
    Dwarf_Unsigned i;
	Dwarf_Off ret_offset;
    res = dwarf_attrlist(die,&attrbuf,&attrcount,&error);
    if(res != DW_DLV_OK) {
        return;
    }
    sf->srcfilesres = dwarf_srcfiles(die,&sf->srcfiles,&sf->srcfilescount, 
        &error);
    for(i = 0; i < attrcount ; ++i) {
        Dwarf_Half aform;
        res = dwarf_whatattr(attrbuf[i],&aform,&error);
        if(res == DW_DLV_OK) {
            if(aform == DW_AT_comp_dir) {
               char *name = 0;
               res = dwarf_formstring(attrbuf[i],&name,&error);
               if(res == DW_DLV_OK) {
              //     printf(    "<%3d> compilation directory : %s\n",level,name);
               }
            }
            if(aform == DW_AT_stmt_list) {
                /* Offset of stmt list for this CU in .debug_line */
				//printf("\nOffset in .debug_line");
				//res = dwarf_dieoffset(die,&ret_offset,&error);
				//printf("\nOffset = %d\n",ret_offset);
            }
        }
        dwarf_dealloc(dbg,attrbuf[i],DW_DLA_ATTR);
    }
    dwarf_dealloc(dbg,attrbuf,DW_DLA_LIST);
}

// This function resets the values in srcfilesdata structure. 
void resetsrcfiles(Dwarf_Debug dbg,struct srcfilesdata *sf)
{
     Dwarf_Signed sri = 0;
     for (sri = 0; sri < sf->srcfilescount; ++sri) {
         dwarf_dealloc(dbg, sf->srcfiles[sri], DW_DLA_STRING);
     }
     dwarf_dealloc(dbg, sf->srcfiles, DW_DLA_LIST);
     sf->srcfilesres = DW_DLV_ERROR;
     sf->srcfiles = 0;
     sf->srcfilescount = 0;
}
void get_addr(Dwarf_Attribute attr,Dwarf_Addr *val)
{
    Dwarf_Error error = 0;
    int res;
    Dwarf_Addr uval = 0;
    res = dwarf_formaddr(attr,&uval,&error);
    if(res == DW_DLV_OK) {
        *val = uval;
        return;
    }
    return;
}
void get_number(Dwarf_Attribute attr,Dwarf_Unsigned *val)
{
    Dwarf_Error error = 0;
    int res;
    Dwarf_Signed sval = 0;
    Dwarf_Unsigned uval = 0;
    res = dwarf_formudata(attr,&uval,&error);
    if(res == DW_DLV_OK) {
        *val = uval;
        return;
    }
    res = dwarf_formsdata(attr,&sval,&error);
    if(res == DW_DLV_OK) {
        *val = sval;
        return;
    }
    return;
}

/*

A strcpy which ensures NUL terminated string
and never overruns the output.

*/
void safe_strcpy(char *out, long outlen, const char *in, long inlen)
{
    if (inlen >= (outlen - 1)) {
        strncpy(out, in, outlen - 1);
        out[outlen - 1] = 0;
    } else {
        strcpy(out, in);
    }
}

void convert_to_upper(char *StrText)
{
  int i = 0;
	for(i = 0 ; i <= strlen(StrText) ; i++)
	{
		if(StrText[i] >= 'a' && StrText[i] <= 'z')
		      StrText[i] -= 32 ;
	}
}
// Function to check the CRC match for the functions in CRC32FunLoc array. 
Func_Loc* check_crc_match(unsigned long SymCRC32)
{
    Func_Loc    *pFunLoc = NULL;
    Func_Loc    *pFunDup = NULL;
    long         LoEle, HiEle, MiEle;
	/*--------------------------------------------------------------------*
	 *  Binary Search the Function Locators using the CRC32 Array         *
	 *--------------------------------------------------------------------*/
	/*--------------------------------------------------------------------*
	 *  If the List contains a single element evaluate it directly        *
	 *--------------------------------------------------------------------*/
	 if (FndFunLoc == 1) {
		 pFunLoc = CRC32FunLoc[0];
		if (SymCRC32 != pFunLoc->SymCRC32) pFunLoc = NULL;
	 }
	 /*--------------------------------------------------------------------*
	  *  For multiple elements perform a Binary Search                     *
	  *--------------------------------------------------------------------*/
  	  else {
			LoEle = 0;                    /* Array low Element     */
			HiEle = FndFunLoc;            /* Array high element    */
			while (LoEle <= HiEle) {
			  MiEle = (LoEle + HiEle)/2; /* Compute Mid-Point     */
			  pFunLoc = CRC32FunLoc[MiEle]; /* Mid-point of scan  */
			 // printf("\nCRC to match is %ld", SymCRC32);
			 // printf("\nCRC checking is %ld,fn is %s", pFunLoc->SymCRC32,pFunLoc->SName);
			  if (SymCRC32 == pFunLoc->SymCRC32) {
				   break;
			  }
	  		  if (SymCRC32 < pFunLoc->SymCRC32) HiEle = MiEle - 1;
			  else LoEle = MiEle+1;
			}
			/*--------------------------------------------------------------------*
			 *  If the CRC values do not match no entry was found                 *
			 *--------------------------------------------------------------------*/
			 if (SymCRC32 != pFunLoc->SymCRC32) {
				printf("\n Entry not found in CRC array\n ");
				pFunLoc = NULL;
			 }
			 /*--------------------------------------------------------------------*
			  *  Search for Multiple Function Names with different Offsets.        *
			  *  If found select the names in ascending offset order.              *
			  *--------------------------------------------------------------------*/
			 /*--------------------------------------------------------------------*
			  *  Find the lowest Locator Entry if not at Start of Array            *
			  *--------------------------------------------------------------------*/
			  else {
				if (MiEle > 0) {
				   LoEle = MiEle - 1;
				   for (pFunDup = CRC32FunLoc[LoEle];
						LoEle > -1 && pFunDup->SymCRC32 == pFunLoc->SymCRC32;
						pFunDup = CRC32FunLoc[--LoEle]); /* NULL Body */
			  		    LoEle++; /* Adjust to point to last with same CRC */
				}
				else
				   LoEle = MiEle;
				   /*--------------------------------------------------------------------*
					*  Find the Highest Locator Entry if not at End of Array             *
					*--------------------------------------------------------------------*/
					if (MiEle != FndFunLoc) {
					   HiEle = MiEle + 1;
					   for (pFunDup = CRC32FunLoc[HiEle];
							 HiEle < FndFunLoc && pFunDup->SymCRC32 == pFunLoc->SymCRC32;
							 pFunDup = CRC32FunLoc[++HiEle]); /* NULL Body */
					}
					/*--------------------------------------------------------------------*
					 *  If more than one entry scan for first unused                      *
					 *--------------------------------------------------------------------*/
					 if (LoEle != HiEle) {
						for (pFunDup = CRC32FunLoc[LoEle];
							LoEle < HiEle && pFunDup->FuncType == 'F';
							pFunDup = CRC32FunLoc[++LoEle]); /* NULL Body */
						if (pFunLoc != pFunDup) pFunLoc = pFunDup;
					 }
				 }
		   }
   return pFunLoc;
}

int search_function_in_file(char **pFileName)
{
	int len;
	Func_Loc  *pFun;
	int iCounter = 0;

    int function_in_file = 0;
	// Check whether this function is already added in the file
	// Search for the file name in the list
	len = strlen(*pFileName);
	len = len > 27 ? 27 : len;
	for (pSrcLoc = pSrc1st, pSrcPrv = pSrc1st;
	     pSrcLoc != NULL && memcmp(pSrcLoc->MemName,*pFileName,len) != 0;
	     pSrcPrv = pSrcLoc, pSrcLoc = pSrcLoc->pNext)
	    ;  /* NULL Loop body                              */
	if (pSrcLoc != NULL) // file name got in the list, now check for the function name
	{
		//printf("\nFile name got = %s",pSrcLoc->MemName);
		len = strlen(pFunLoc->SName);
	    for(pFun = pSrcLoc->pFunHoC ,iCounter=0;iCounter < pSrcLoc->RefCnt;
		      pFun = pFun->pSrcFun,iCounter++)
	    {
			if(memcmp(pFun->SName,pFunLoc->SName,len) == 0)
		//		printf("\nFirst Line numbers are %d %d", pFun->FLNum,pFunLoc->FLNum);
		  if(memcmp(pFun->SName,pFunLoc->SName,len) == 0 && (pFun->FLNum == pFunLoc->FLNum) ) // If overloaded functions are there, then they might be missed, so check the line number also ?
		  {
			//  printf("\nThe fn %s already in %s file list",pFun->SName,*pFileName);
			  function_in_file = 1;

		  }
 	    }
	}
 return function_in_file;
}



