/*
   AngelCode Scripting Library
   Copyright (c) 2024 Andreas Jonsson

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_callfunc_riscv64.cpp
//
// These functions handle the actual calling of system functions  
// on the 64bit RISC-V call convention used for Linux
//


#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_RISCV64

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"
#include "as_context.h"

BEGIN_AS_NAMESPACE

// retfloat == 0: the called function doesn't return a float value
// retfloat == 1: the called function returns a float/double value
// argValues is an array with all the values, the first 8 values will go to a0-a7 registers, the next 8 values will go to fa0-fa7 registers, and the remaining goes to the stack
// numRegularValues holds the number of regular values to put in a0-a7 registers
// numFloatValues hold the number of float values to put in fa0-fa7 registers
// numStackValues hold the number of values to push on the stack
extern "C" asQWORD CallRiscVFunc(asFUNCTION_t func, int retfloat, asQWORD *argValues, int numRegularValues, int numFloatValues, int numStackValues);

asQWORD CallSystemFunctionNative(asCContext *context, asCScriptFunction *descr, void *obj, asDWORD *args, void *retPointer, asQWORD &retQW2, void *secondObject)
{
	asCScriptEngine *engine = context->m_engine;
	const asSSystemFunctionInterface *const sysFunc = descr->sysFuncIntf;
	const asCDataType &retType = descr->returnType;
	const asCTypeInfo *const retTypeInfo = retType.GetTypeInfo();
	asFUNCTION_t func = sysFunc->func;
	int callConv = sysFunc->callConv;
	asQWORD retQW = 0;

	// TODO: retrieve correct function pointer to call (e.g. from virtual function table, auxiliary pointer, etc)

	// Prepare the values that will be sent to the native function
	// a0-a7 used for non-float values
	// fa0-fa7 used for float values
	// if more than 8 float values and there is space left in regular registers then those are used
	// rest of the values are pushed on the stack
	const int maxRegularRegisters = 8;
	const int maxFloatRegisters = 8;
	const int maxValuesOnStack = 64 - maxRegularRegisters - maxFloatRegisters;
	asQWORD argValues[maxRegularRegisters + maxFloatRegisters + maxValuesOnStack];
	asQWORD* stackValues = argValues + maxRegularRegisters + maxFloatRegisters;
	
	int numRegularRegistersUsed = 0;
	int numFloatRegistersUsed = 0;
	int numStackValuesUsed = 0;
	asUINT argsPos = 0;
	for (asUINT n = 0; n < descr->parameterTypes.GetLength(); n++)
	{
		const asCDataType& parmType = descr->parameterTypes[n];

		// TODO: Check for object types
		// TODO: Check for question type
		// TODO: Check for float types
		if (parmType.IsReference() || parmType.IsObjectHandle() || parmType.IsIntegerType() || parmType.IsUnsignedType() || parmType.IsBooleanType() )
		{
			// pointers, integers, and booleans go to regular registers
			const asUINT parmDWords = parmType.GetSizeOnStackDWords();
			if (numRegularRegistersUsed < maxRegularRegisters)
			{
				if (parmDWords == 1)
					argValues[numRegularRegistersUsed] = (asQWORD)args[argsPos];
				else
					argValues[numRegularRegistersUsed] = *(asQWORD*)&args[argsPos];
				numRegularRegistersUsed++;
			}
			else (numStackValuesUsed < maxValuesOnStack)
			{
				// TODO: Is the values on the stack DWORD aligned or QWORD aligned?
				if( parmDWords == 1 )
					stackValues[numStackValuesUsed] = (asQWORD)args[argsPos];
				else
					stackValues[numStackValuesUsed] = *(asQWORD*)&args[argsPos];
				numStackValuesUsed++;
			}
			else
			{
				// Oops, we ran out of space in the argValues array!
				// TODO: This should be validated as the function is registered
				asASSERT(false);
			}
			argsPos += parmDWords;
		}
	}

	int retfloat = sysFunc->hostReturnFloat ? 1 : 0;
	retQW = CallRiscVFunc(func, retfloat, argValues, numRegularRegistersUsed, numFloatRegistersUsed, numStackValuesUsed);

	return retQW;
}

END_AS_NAMESPACE

#endif // AS_RISCV64
#endif // AS_MAX_PORTABILITY



