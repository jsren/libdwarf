#include <vector>
#include "const.h"
#include "dwarf.h"

namespace dwarf
{

    class ExpressionParser
    {
    private:
		std::vector<int64_t> stack;
		std::vector<uint8_t*> ipstack;

		std::size_t addressSize;
		std::size_t dwarfWidth;

        int64_t(*readRegister)(uint64_t);
        int64_t(*getFrameBase)();
        int64_t(*readAddress)(uint64_t);
        int64_t(*readAddressEx)(uint64_t, uint64_t);
        int64_t(*getObjectAddress)();
        int64_t(*getTLSAddress)(uint64_t);
        int64_t(*getCallFrameCFA)();


        inline auto stack_front() { return stack[stack.size() - 1]; }

        template<typename T>
        inline void stack_push(T&& value) { stack.push_back(std::forward<T>(value)); }

        inline void stack_pop() { stack.erase(stack.begin()+(stack.size()-1)); }

        inline auto& stack_at(std::size_t index) { return stack[stack.size() - index - 1]; }

        template<typename T>
        inline void stack_insert(std::size_t index, T&& value) { 
            stack.insert(stack.begin() + (stack.size() - index - 1), std::forward<T>(value)); 
        }


    private:
        int64_t runNext(OpCode code, const uint8_t* operands, std::size_t length)
        {
            auto intcode = static_cast<uint32_t>(code);

            // Handle push literal
            if (code >= OpCode::Lit0 && code <= OpCode::Lit31) {
                stack_push(intcode - static_cast<uint32_t>(OpCode::Lit0));
            }
            // Handle push address
            else if (code == OpCode::Address) {
                uint64_t value; memcpy(&value, operands, addressSize); 
                stack_push(value); return addressSize;
            }
            // Handle push unsigned constant
            else if (code == OpCode::Const1U || code == OpCode::Const2U ||
                code == OpCode::Const4U || code == OpCode::Const8U)
            {
                auto size = 1 << ((intcode - static_cast<uint32_t>(OpCode::Const1U)) / 2);
                uint64_t value; memcpy(&value, operands, size);
                stack_push(value); return size;
            }
            // Handle push unsigned constant
            else if (code == OpCode::Const1S || code == OpCode::Const2S ||
                code == OpCode::Const4S || code == OpCode::Const8S)
            {
                auto size = 1 << ((intcode - static_cast<uint32_t>(OpCode::Const1S)) / 2);
                int64_t value; memcpy(&value, operands, size);
                // Perform manual sign extension
                if (value & (1 << (size * 8 - 1))) { value |= static_cast<uint64_t>(-1) << (size * 8); }
                stack_push(value); return size;
            }
            // Handle push ULEB constant
            else if (code == OpCode::ConstU)
            {
                uint64_t value; auto size = uleb_read(operands, value);
                stack_push(value); return size;
            }
            // Handle push LEB constant
            else if (code == OpCode::ConstS)
            {
                int64_t value; auto size = sleb_read(operands, value);
                stack_push(value); return size;
            }
            // Handle push frame base + offset
            else if (code == OpCode::FBReg)
            {
                int64_t value; auto size = sleb_read(operands, value);
                stack_push(value + getFrameBase()); return size;
            }
            // TODO: Handle reg literal + offset
            // Handle push reg + offset
            else if (code == OpCode::BRegX)
            {
                uint64_t reg; auto size = uleb_read(operands, reg);
                int64_t value; size += sleb_read(operands + size, value);
                stack_push(value + readRegister(reg)); return size;
            }
            // Handle push reg + offset
            else if (code == OpCode::BRegX)
            {
                uint64_t reg; auto size = uleb_read(operands, reg);
                int64_t value; size += sleb_read(operands + size, value);
                stack_push(value + readRegister(reg)); return size;
            }
            // Handle stack operations
            else if (code == OpCode::Dup) {
                stack_push(stack_front());
            }
            else if (code == OpCode::Drop) {
                stack_pop();
            }
            else if (code == OpCode::Pick) {
                uint8_t index = operands[0]; stack_push(stack_at(index)); return 1;
            }
            else if (code == OpCode::Over) {
                stack_push(stack_at(1));
            }
            else if (code == OpCode::Swap) {
                auto i1 = stack.front(); stack_pop();
                auto i2 = stack.front(); stack_pop();
                stack_push(i1); stack_push(i2);
            }
            else if (code == OpCode::Rot) {
                auto i1 = stack_front(); stack_pop();
                stack_insert(2, i1);
            }
            // Handle external value operations
            else if (code == OpCode::Deref) {
                auto value = readAddress(static_cast<uint64_t>(stack.front())); 
                stack_pop(); stack_push(value);
            }
            else if (code == OpCode::DerefSize) {
                uint8_t vsize = operands[0];
                uint64_t i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                uint64_t value = readAddress(i1);
                value &= ~(static_cast<uint64_t>(-1) << (vsize * 8));
                stack_push(value);
                return 1;
            }
            else if (code == OpCode::XDeref) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto value = readAddressEx(i1, i2); stack_push(value);
            }
            else if (code == OpCode::XDerefSize) {
                uint8_t vsize = operands[0];
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                uint64_t value = readAddressEx(i1, i2);
                value &= ~(static_cast<uint64_t>(-1) << (vsize * 8));
                stack_push(value);
                return 1;
            }
            else if (code == OpCode::PushObjectAddress) {
                stack_push(getObjectAddress());
            }
            else if (code == OpCode::FormTLSAddress) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(getTLSAddress(i1));
            }
            else if (code == OpCode::CallFrameCFA) {
                stack_push(getCallFrameCFA());
            }
            // Handle arithmetic/logic operations
            else if (code == OpCode::Abs) {
                stack_at(0) = stack_at(0) < 0 ? -stack_at(0) : stack_at(0);
            }
            else if (code == OpCode::And) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(i1 & i2);
            }
            else if (code == OpCode::Div) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(i2 / i1);
            }
            else if (code == OpCode::Minus) {
                auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
                stack_push(i2 - i1);
            }
            else if (code == OpCode::Mod) {
                auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
                stack_push(i2 % i1);
            }
            else if (code == OpCode::Mul) {
                auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
                stack_push(i2 * i1);
            }
            else if (code == OpCode::Neg) {
                stack_at(0) = -stack_at(0);
            }
            else if (code == OpCode::Not) {
                stack_at(0) = ~stack_at(0);
            }
            else if (code == OpCode::Or) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(i2 | i1);
            }
            else if (code == OpCode::Plus) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(i2 + i1);
            }
            else if (code == OpCode::PlusUConst) {
                auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
                uint64_t value; auto size = uleb_read(operands, value);
                stack_push(i1 + value);
                return size;
            }
            else if (code == OpCode::Shl) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(i2 << i1);
            }
            else if (code == OpCode::Shr) {
                auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
                stack_push(i2 >> i1);
            }
            else if (code == OpCode::Shra) {
                auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
                auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
                stack_push(i2 / (1 << i1));
            }
			else if (code == OpCode::Xor) {
				auto i1 = static_cast<uint64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<uint64_t>(stack.front()); stack_pop();
				stack_push(i2 ^ i1);
			}
			// Handle signed comparisons
			else if (code == OpCode::Le) {
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
				stack_push(i2 <= i1 ? 1 : 0);
			}
			else if (code == OpCode::Ge) {
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
				stack_push(i2 >= i1 ? 1 : 0);
			}
			else if (code == OpCode::Eq) {
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
				stack_push(i2 == i1 ? 1 : 0);
			}
			else if (code == OpCode::Lt) {
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
				stack_push(i2 < i1 ? 1 : 0);
			}
			else if (code == OpCode::Gt) {
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
				stack_push(i2 > i1 ? 1 : 0);
			}
			else if (code == OpCode::Ne) {
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();
				auto i2 = static_cast<int64_t>(stack.front()); stack_pop();
				stack_push(i2 != i1 ? 1 : 0);
			}
			// Handle control flow operations
			else if (code == OpCode::Skip)
			{
				int16_t offset; memcpy(&offset, operands, 2);
				return 2 + offset;
			}
			else if (code == OpCode::Bra)
			{
				int16_t offset = 0;
				auto i1 = static_cast<int64_t>(stack.front()); stack_pop();

				if (i1) { memcpy(&offset, operands, 2); }
				return 2 + offset;
			}
			else if (code == OpCode::Call2)
			{
				//ipstack.push_back()
			}

			// Handle NOP
			else if (code == OpCode::Nop) { }

            return 0;
        }

    };
}
