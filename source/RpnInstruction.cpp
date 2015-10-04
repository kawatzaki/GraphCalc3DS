#include "RpnInstruction.h"
#include <cmath>

RpnInstruction::RpnInstruction(Opcode opcode)
{
	op = opcode;
}

RpnInstruction::RpnInstruction(float value)
{
	op = OP_PUSH;
	this->value = value;
}

RpnInstruction::RpnInstruction(float *var)
{
	op = OP_PUSHVAR;
	this->var = var;
}

RpnInstruction::RpnInstruction(func_t func, const char *name)
{
	op = OP_FUNCTION;
	this->func = func;
	this->name = name;
}

RpnInstruction::Status RpnInstruction::Execute(std::vector<float> &stack) const
{
	switch (op) {
		case OP_PUSH:
			stack.push_back(value);
			return S_OK;
		case OP_PUSHVAR:
			stack.push_back(*var);
			return S_OK;
		case OP_ADD:
			if (stack.size() < 2) {
				return S_UNDERFLOW;
			} else {
				float x = stack.back();
				stack.pop_back();
				float y = stack.back();
				stack.pop_back();
				stack.push_back(x + y);
				return S_OK;
			}
		case OP_SUBTRACT:
			if (stack.size() < 2) {
				return S_UNDERFLOW;
			} else {
				float x = stack.back();
				stack.pop_back();
				float y = stack.back();
				stack.pop_back();
				stack.push_back(x - y);
				return S_OK;
			}
		case OP_MULTIPLY:
			if (stack.size() < 2) {
				return S_UNDERFLOW;
			} else {
				float x = stack.back();
				stack.pop_back();
				float y = stack.back();
				stack.pop_back();
				stack.push_back(x * y);
				return S_OK;
			}
		case OP_DIVIDE:
			if (stack.size() < 2) {
				return S_UNDERFLOW;
			} else {
				float x = stack.back();
				stack.pop_back();
				float y = stack.back();
				stack.pop_back();
				if (y == 0) {
					return S_UNDEFINED;
				} else {
					stack.push_back(x / y);
					return S_OK;
				}
			}
		case OP_MODULO:
			if (stack.size() < 2) {
				return S_UNDERFLOW;
			} else {
				float x = stack.back();
				stack.pop_back();
				float y = stack.back();
				stack.pop_back();
				if (y == 0) {
					return S_UNDEFINED;
				} else {
					stack.push_back(std::fmod(x, y));
					return S_OK;
				}
			}
		case OP_POWER:
			if (stack.size() < 2) {
				return S_UNDERFLOW;
			} else {
				float x = stack.back();
				stack.pop_back();
				float y = stack.back();
				stack.pop_back();
				stack.push_back(std::pow(x, y));
				return S_OK;
			}
		case OP_NEGATE:
			if (stack.size() == 0) {
				return S_UNDERFLOW;
			} else {
				float &back = stack.back();
				back = -back;
				return S_OK;
			}
		case OP_FUNCTION:
			if (stack.size() == 0) {
				return S_UNDERFLOW;
			} else {
				float &back = stack.back();
				back = func(back);
				return S_OK;
			}
		default:
			return S_UNDEFINED;
	}
}

std::ostream &operator<<(std::ostream &os, const RpnInstruction &inst)
{
	switch (inst.op) {
		case RpnInstruction::OP_PUSH:
			os << inst.value;
			break;
		case RpnInstruction::OP_ADD:
			os << '+';
			break;
		case RpnInstruction::OP_SUBTRACT:
			os << '-';
			break;
		case RpnInstruction::OP_MULTIPLY:
			os << '*';
			break;
		case RpnInstruction::OP_DIVIDE:
			os << '\xF6';
			break;
		case RpnInstruction::OP_MODULO:
			os << "mod";
			break;
		case RpnInstruction::OP_POWER:
			os << '^';
			break;
		case RpnInstruction::OP_NEGATE:
			os << '\xF1';
			break;
		case RpnInstruction::OP_FUNCTION:
			os << inst.name;
			break;
		default:
			os << "???";
	}
	return os;
}

RpnInstruction::Status ExecuteRpn(const std::vector<RpnInstruction> instructions, float &resultOut)
{
	std::vector<float> stack;
	
	for (auto i = instructions.begin(); i != instructions.end(); i++) {
		RpnInstruction::Status status = i->Execute(stack);
		if (status != RpnInstruction::S_OK) {
			return status;
		}
	}
	
	if (stack.size() == 0) {
		return RpnInstruction::S_UNDERFLOW;
	} else {
		resultOut = stack.back();
		return RpnInstruction::S_OK;
	}
}