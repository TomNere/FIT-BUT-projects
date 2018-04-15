"""
#  Course:      Principles of Programming Languages (IPP)
#  Project:     Interpret of IPPcode18 XML representation
#  File:        interpret.py
#
#  Author: Tomáš Nereča : xnerec00
"""


import argparse                     # Parsing arguments
import sys                          # Printing to STDERR
import re                           # Regular expressions
import xml.etree.ElementTree as ET  # Parsing XML

"""""""""""""""""""""""""""""""""ERROR_CODES"""""""""""""""""""""""""""""""""""
XML_ERR = 31
SYN_ERR = 32
ARG_ERR = 10
IN_ERR = 11
SEM_ERR = 52
TYPE_ERR = 53
VAR_ERR = 54
FRAME_ERR = 55
VOID_ERROR = 56
ZERO_ERR = 57
STR_ERR = 58
OTHER_ERR = 99


"""""""""""""""""""""""""""""""""""""HELPING_FUNCTIONS"""""""""""""""""""""""""""""""""""""""""""


def retError(err, msg):
    # Print error message to stderr and exit
    sys.stderr.write("Error found in instruction number " + str(allInst.getCounter()) + ".\n")
    sys.stderr.write(msg + ".\n")
    exit(err)


def nameCheck(name):
    # Check if name of variable or label is valid
    if re.match('^[a-zA-Z_\-$&%*]*$', name[0]):             # First character musn't be digit
        if not re.match('^[a-zA-Z0-9_\-$&%*]*$', name[1:]):
            retError(SYN_ERR, 'Invalid character in name of var/label')
    else:
        retError(SYN_ERR, 'Invalid character in name of var/label')


def getFrame(frame):
    # Check for frame name and return proper data structure
    global global_frame, tmp_frame, frame_stack

    if frame == 'GF':
        return global_frame
    elif frame == 'LF':
        if not frame_stack:             # Check if LF is defined
            retError(FRAME_ERR, 'Undefined local frame')
        return frame_stack[-1]
    elif frame == 'TF':                 # Check if TF is defined
        if tmp_frame['undefined'] is True:
            retError(FRAME_ERR, 'Undefined temporary frame')
        return tmp_frame
    else:
        retError(SYN_ERR, 'Invalid variable name')


def varTranslate(var):
    # Translate variable if variable, else return constant
    if var['type'] == 'bool' or var['type'] == 'int' or var['type'] == 'string' or var['type'] == 'type':
        new_var = dict(var)         # Need to make a copy not only reference
        typeCheck(new_var)
        return new_var
    elif var['type'] == 'var':
        frame = getFrame(var['value'][:2])
        if var['value'][3:] in frame:
            if var['type'] is None:     # Variable is declared but undefined
                retError(VOID_ERROR, "Undefined variable")
            return frame[var['value'][3:]]
        else:
            retError(VAR_ERR, "Undeclared variable")
    else:                                               # Label can't be here
        retError(SYN_ERR, "Unknown type of symbol")


def opCheck(inst, number):
    # Check number of operands
    if len(inst['args']) > 3 or len(inst['args']) != number:
        retError(XML_ERR, 'Invalid number of operands')


def assignValue(var, new_value):
    # Assign new_value to variable
    if var['type'] != 'var':
        retError(TYPE_ERR, 'Wrong type of operand')

    frame = getFrame(var['value'][:2])
    if var['value'][3:] in frame:           # If variable exists in frame update, else error
        frame.update({var['value'][3:]: new_value})
    else:
        retError(VAR_ERR, 'Undeclared variable')


def typeCheck(var):
    # Check if constant is valid
    if var['type'] == 'int':
        if var['value'] is None:            # Void number
            retError(SYN_ERR, 'Integer operand without value')
        number = intCheck(var['value'])     # Check for valid number
        if number is False:
            retError(SYN_ERR, "Invalid integer value")
        var.update({'value': number})       # Assign converted value
    elif var['type'] == 'bool':
        if var['value'] is None:            # Void boolean
            retError(SYN_ERR, 'Boolean operand without value')
        if var['value'] == 'true':
            var.update({'value': True})     # Assign True
        elif var['value'] == 'false':
            var.update({'value': False})    # Assign False
        else:
            retError(SYN_ERR, "Invalid bool value")
    elif var['type'] == 'type':             # Type
        if var['value'] != 'int' and var['value'] != 'string' and var['value'] != 'bool':
            retError(SYN_ERR, 'Invalid type')
    else:                                   # String
        var.update({'value': strCheck(var['value'])})


def tryInput():
    # Try to read from stdin and catch exception
    try:
        var = input()
    except EOFError:        # If EOF, false
        var = False
    return var


def intCheck(var):
    # Try to convert string to decimal integer and catch exception
    try:
        integer = int(var, base=10)
    except ValueError:              # Invalid integer
        integer = False
    return integer


def strCheck(var):
    # Check for valid string and translate escape sequences
    if var is None:     # Empty string
        return ""

    new_string = ""
    counter = 0
    while counter < len(var):
        if var[counter] == '\\':    # Escape sequence
            number = var[counter + 1:counter + 4]   # \000
            if number.isdigit():                    # Must be digit 000-999
                new_string += chr(int(number, base=10))     # Translate
                counter += 3                        # Skip number
            else:                                   # Invalid escape sequence
                retError(SYN_ERR, 'Bad escape sequence in string')
        elif var[counter].isspace():                # Blank character have to be escaped
            retError(SYN_ERR, 'Unknown character in string')
        else:                                       # Valid character, concatenate
            new_string += var[counter]
        counter += 1
    return new_string


"""""""""""""""""""""""""""""""""""""INSTRUCTIONS"""""""""""""""""""""""""""""""""""""""""""


def inothing(inst):
    # Nothing to do
    pass


def iframes(inst):
    # Instructions with frames
    opCheck(inst, 0)
    global tmp_frame
    global frame_stack

    if inst['opcode'] == 'CREATEFRAME':
        tmp_frame.clear()                       # Clear old TF
        tmp_frame = {'undefined': False}        # TF is now defined

    elif inst['opcode'] == 'PUSHFRAME':
        if tmp_frame['undefined'] is True:      # TF doesn't exist
            retError(FRAME_ERR, 'Undefined temporary frame')
        frame_stack.append(dict(tmp_frame))     # Push frame to stack of frames
        tmp_frame.clear()                       # Clean TF
        tmp_frame = {'undefined': True}         # TF is now undefined

    elif inst['opcode'] == 'POPFRAME':
        if not frame_stack:                     # Empty stack of frames
            retError(FRAME_ERR, 'Stack of frames is empty')
        if tmp_frame['undefined'] is True:      # TF doesn't exist
            retError(FRAME_ERR, 'Undefined temporary frame')
        tmp_frame = dict(frame_stack[-1])       # Assign LF to TF
        del frame_stack[-1]                     # Delete old LF


def ireturn(inst):
    # Instruction return
    opCheck(inst, 0)
    global call_stack

    if not call_stack:      # Empty stack of calls
        retError(VOID_ERROR, 'Empty call stack')

    allInst.setCounter(call_stack[-1])  # Set program counter from top of stack
    del call_stack[-1]                  # Delete value on the top of stack


def ibreak(inst):
    # Instruction break
    opCheck(inst, 0)
    global global_frame, tmp_frame, frame_stack

    # Print content of GF
    sys.stderr.write('Global frame: ' + str(global_frame) + "\n" + 'Local frame: ')

    # Check if LF exist's
    if not frame_stack:
        sys.stderr.write('empty')
    else:
        sys.stderr.write(str(frame_stack[-1]))      # Print content of LF

    # Print content of TF
    sys.stderr.write("\n" + 'Tmp frame: ' + str(tmp_frame) + "\n")


def idefvar(inst):
    # Instruction defvar
    opCheck(inst, 1)
    name = inst['args'][0]['value']     # Name of variable
    frame = getFrame(name[:2])          # Get frame
    name = name[3:]
    nameCheck(name)                     # Check for valid variable name
    if name in frame:       # Variable exist's in frame
        retError(SEM_ERR, 'Redefinition of variable')
    frame.update({name: {'type': None, 'value': None}})     # Create new undefined variable in frame


def icall(inst):
    # Instruction call
    opCheck(inst, 1)
    global call_stack, allInst
    call_stack.append(int(allInst.getCounter()))    # Push copy of program counter to stack of calls

    if inst['args'][0]['type'] == 'label':          # Must be label
        if inst['args'][0]['value'] in label_arr:   # Label exists
            allInst.setCounter(label_arr[inst['args'][0]['value']])     # Set new program counter
        else:
            retError(SEM_ERR, 'Undefined label.')
    else:
        retError(SYN_ERR, 'Label expected')


def ipushs(inst):
    # Instruction pushs - push value on the top of data stack
    opCheck(inst, 1)
    var = varTranslate(inst['args'][0])             # Translate variable name to value
    global data_stack
    data_stack.append(dict(var))                    # Push copy of variable to the top of data stack


def ipops(inst):
    # Instruction pops - pop value from data stack
    opCheck(inst, 1)
    global data_stack

    if not data_stack:                              # Check if data stack is empty
        retError(VOID_ERROR, 'Empty data stack')
    else:
        assignValue(inst['args'][0], data_stack[-1])
        del data_stack[-1]


def iwrite(inst):
    # Instruction write
    opCheck(inst, 1)
    var = varTranslate(inst['args'][0])
    printed = ""
    if var['type'] == 'int':                # Int to str
        printed = str(var['value'])
    elif var['type'] == 'bool':             # Bool values translate
        if var['value'] is True:
            printed = 'true'
        else:
            printed = 'false'
    elif var['type'] == 'string' or var['type'] == 'type':
        printed = var['value']              # Simply print value
    elif var['type'] == 'label':            # Label forbidden
        retError(XML_ERR, 'Invalid type for write')
    else:                                   # Undefined
        if inst['opcode'] == 'WRITE':
            retError(VOID_ERROR, 'Undefined variable')
        else:
            sys.stderr.write("Undefined variable.\n")
            return

    if inst['opcode'] == 'WRITE':   # Write
        print(printed)
    else:                           # Dprint
        sys.stderr.write(str(printed))


def ijump(inst):
    # Instruction jump
    opCheck(inst, 1)
    global label_arr, allInst
    if inst['args'][0]['value'] in label_arr:       # Check  if label exist
        allInst.setCounter(label_arr[inst['args'][0]['value']])     # Set program counter
    else:
        retError(SEM_ERR, 'Undefined label')


def imove(inst):
    # Instruction move
    opCheck(2)
    var = varTranslate(inst['args'][1])
    if var['type'] == 'type':               # Forbidden
        retError(TYPE_ERR, 'Wrong type of operand')
    assignValue(inst['args'][0], var)       # Call function which assign value


def iint2char(inst):
    # Instruction int2char
    opCheck(inst, 2)
    var = varTranslate(inst['args'][1])
    char = ""
    if var['type'] != 'int':        # Must be integer
        retError(TYPE_ERR, 'Integer expected')

    try:                            # Try to convert
        char = chr(var['value'])
    except ValueError:              # Unable to convert
        retError(STR_ERR, "Invalid Unicode value")

    assignValue(inst['args'][0], {'type': 'string', 'value': char})


def iread(inst):
    # Instruction read
    opCheck(inst, 2)
    if inst['args'][1]['type'] != 'type':       # Second operand has to be type
        retError(TYPE_ERR, 'Type expected')

    var = tryInput()
    if inst['args'][1]['value'] == 'bool':
        if var == 'true':                       # If true, assign True, else False
            var = True
        else:
            var = False
    elif inst['args'][1]['value'] == 'string':
        if var is False:                        # If False, assign '', else leave string
            var = ''
    elif inst['args'][1]['value'] == 'int':
        if var is False:                        # If False, assign 0
            var = 0
        else:
            var = intCheck(var)
            if var is False:                    # Invalid integer, assign 0
                var = 0
    else:
        retError(SYN_ERR, 'Unknown type')

    assignValue(inst['args'][0], {'type': inst['args'][1]['value'], 'value': var})


def istrlen(inst):
    # Instruction strlen
    opCheck(inst, 2)
    string = varTranslate(inst['args'][1])
    if string['type'] != 'string':              # Must be string
        retError(TYPE_ERR, 'String expected')
    length = len(string['value'])               # Get length of string
    assignValue(inst['args'][0], {'type': 'int', 'value': length})


def itype(inst):
    # Instruction type
    opCheck(inst, 2)
    var = varTranslate(inst['args'][1])
    if var['type'] is None:             # Undefined variable
        typee = ''
    else:                               # Defined, assign type
        typee = var['type']
    assignValue(inst['args'][0], {'type': 'string', 'value': typee})


def ievaluation(inst):
    # Instructions add, sub, mul, idiv
    opCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])
    number = 0

    if op1['type'] == 'int' and op2['type'] == 'int':   # Must be numbers
        if inst['opcode'] == 'ADD':                     # Add
            number = op1['value'] + op2['value']
        elif inst['opcode'] == 'SUB':                   # Sub
            number = op1['value'] - op2['value']
        elif inst['opcode'] == 'MUL':                   # Mul
            number = op1['value'] * op2['value']
        elif inst['opcode'] == 'IDIV':                  # Idiv
            if op2['value'] != 0:                       # Check for division by zero
                number = op1['value'] // op2['value']
            else:
                retError(ZERO_ERR, 'Division by Zero')
        assignValue(inst['args'][0], {'type': 'int', 'value': number})
    else:
        retError(TYPE_ERR, 'Integers expected')


def icomparison(inst):
    # Instructions lt, gt, eq
    opCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])
    flag = False

    # Action for int and string
    if (op1['type'] == 'int' and op2['type'] == 'int') or (op1['type'] == 'string' and op2['type'] == 'string'):
        if inst['opcode'] == 'LT':
            flag = op1['value'] < op2['value']
        elif inst['opcode'] == 'GT':
            flag = op1['value'] > op2['value']
        elif inst['opcode'] == 'EQ':
            flag = op1['value'] == op2['value']

    # Action for bool
    elif op1['type'] == 'bool' and op2['type'] == 'bool':
        # Defined in project specification
        if inst['opcode'] == 'LT':
            flag = op1['value'] is False and op2['value'] is True
        elif inst['opcode'] == 'GT':
            flag = op1['value'] is True and op2['value'] is False
        elif inst['opcode'] == 'EQ':
            flag = op1['value'] is op2['value']
    else:
        retError(TYPE_ERR, 'Invalid type to compare')

    assignValue(inst['args'][0], {'type': 'bool', 'value': flag})


def ilogic(inst):
    # Instruction and, or
    opCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])
    flag = False

    # Operands have to be bool
    if op1['type'] == 'bool' and op2['type'] == 'bool':
        if inst['opcode'] == 'AND':
            flag = op1['value'] and op2['value']
        elif inst['opcode'] == 'OR':
            flag = op2['value'] or op2['value']
    else:
        retError(TYPE_ERR, 'Bool operands expected')

    assignValue(inst['args'][0], {'type': 'bool', 'value': flag})


def inot(inst):
    # Instruction not
    opCheck(inst, 2)
    op1 = varTranslate(inst['args'][1])

    # Operand has to be bool
    if op1['type'] == 'bool':
        assignValue(inst['args'][0], {'type': 'bool', 'value': not op1['value']})
    else:
        retError(TYPE_ERR, 'Bool operand expected')


def istri2int(inst):
    #Instructions stri2int, getchar
    opCheck(inst, 3)
    string = varTranslate(inst['args'][1])
    pos = varTranslate(inst['args'][2])

    if string['type'] == 'string' and pos['type'] == 'int':
        if (pos['value'] < 0) or (pos['value'] > len(string['value']) - 1):     # Check for valid index
            retError(STR_ERR, 'Index out of bounds')
        if inst['opcode'] == 'STRI2INT':
            var = ord(string['value'][pos['value']])                            # Get ordinal value
            assignValue(inst['args'][0], {'type': 'int', 'value': var})
        elif inst['opcode'] == 'GETCHAR':
            var = string['value'][pos['value']]                                 # Get one character
            assignValue(inst['args'][0], {'type': 'string', 'value': var})
    else:
        retError(TYPE_ERR, 'Invalid type of operand(s)')


def iconcat(inst):
    # Instruction concat
    opCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])

    if op1['type'] == 'string' and op2['type'] == 'string':     # Must be strings
        string = op1['value'] + op2['value']
        assignValue(inst['args'][0], {'type': 'string', 'value': string})
    else:
        retError(TYPE_ERR, 'String operands expected')


def isetchar(inst):
    # Instruction setchar
    opCheck(inst, 3)
    pos = varTranslate(inst['args'][1])
    char = varTranslate(inst['args'][2])
    var = False

    # First operand has to be string variable
    if inst['args'][0]['type'] == 'var':
        var = varTranslate(inst['args'][0])
    else:
        retError(TYPE_ERR, 'Variable expected')

    if var['type'] == 'string' and pos['type'] == 'int' and char['type'] == 'string':
        # Check for valid index
        if (pos['value'] < 0) or (pos['value'] > len(var['value']) - 1) or (len(char['value']) == 0):
            retError(STR_ERR, 'Index out of bounds')

        # Do stuff
        var['value'] = var['value'][:pos['value']] + char['value'][0] + var['value'][pos['value'] + 1:]
        assignValue(inst['args'][0], var)
    else:
        retError(TYPE_ERR, 'Invalid type of argument(s)')


def ijumpif(inst):
    # Instructions jumpifeq, jumpifneq
    opCheck(inst, 3)

    var1 = varTranslate(inst['args'][1])
    var2 = varTranslate(inst['args'][2])

    if var1['type'] != var2['type']:                # Operands have to be same type
        retError(TYPE_ERR, "Type incompatibility")

    if inst['args'][0]['value'] in label_arr:       # Check if label exists
        if inst['opcode'] == 'JUMPIFEQ':            # JUMPIFEQ
            if var1 == var2:
                allInst.setCounter(label_arr[inst['args'][0]['value']])
        else:                                       # JUMPIFNEQ
            if var1 != var2:
                allInst.setCounter(label_arr[inst['args'][0]['value']])
    else:
        retError(SEM_ERR, "Undefined label")


"""""""""""""""""""""""""""""""""""""""""""""""""""""MAIN_SWITCH"""""""""""""""""""""""""""""""""""""""""""""""""""


def mainSwitch(inst):
    switcher = {
        'CREATEFRAME': iframes,
        'PUSHFRAME': iframes,
        'POPFRAME': iframes,
        'RETURN': ireturn,
        'BREAK': ibreak,
        'DEFVAR': idefvar,
        'CALL': icall,
        'PUSHS': ipushs,
        'POPS': ipops,
        'WRITE': iwrite,
        'LABEL': inothing,
        'JUMP': ijump,
        'DPRINT': iwrite,
        'MOVE': imove,
        'INT2CHAR': iint2char,
        'READ': iread,
        'STRLEN': istrlen,
        'TYPE': itype,
        'ADD': ievaluation,
        'SUB': ievaluation,
        'MUL': ievaluation,
        'IDIV': ievaluation,
        'LT': icomparison,
        'GT': icomparison,
        'EQ': icomparison,
        'AND': ilogic,
        'OR': ilogic,
        'NOT': inot,
        'STRI2INT': istri2int,
        'CONCAT': iconcat,
        'GETCHAR': istri2int,
        'SETCHAR': isetchar,
        'JUMPIFEQ': ijumpif,
        'JUMPIFNEQ': ijumpif,
    }

    # Call function or exit if opcode is invalid
    func = switcher.get(inst, lambda x: retError(XML_ERR, 'Unknown opcode'))
    func(allInst.getInst())


class InstArr:
    # Class representing list of all instructions
    # Implements also program counter
    def __init__(self):
        self.count = 0              # Number of instructions
        self.instructions = []      # List of instructions
        self.program_counter = 0    # Program counter

    def addInst(self, opcode, args):
        # add instruction to list of all instructions
        instruction = {
            'opcode': opcode,
            'args': args
        }
        self.instructions.append(instruction)
        self.count += 1

    def getInst(self):
        return self.instructions[self.program_counter - 1]

    def incCounter(self):
        self.program_counter += 1

    def setCounter(self, number):
        self.program_counter = number

    def getCounter(self):
        return self.program_counter


def createArr(path):
    global label_arr
    global allInst

    try:
        tree = ET.parse(path)
        root = tree.getroot()
        inst_order = 1
        for child in root:
            if child.tag == 'instruction':              # If instruction, add to array
                if child.attrib['order'] != str(inst_order) or len(child.attrib) > 2:
                    sys.stderr.write('Invalid XML element:'+ str(child) + "\n")
                    exit(XML_ERR)

                args = []
                arg_counter = 1
                for arg in child:                   # Iterate in arguments
                    if arg.tag[0:3] != 'arg' or arg.tag[3] != str(arg_counter) or len(arg.attrib) > 1:
                        sys.stderr.write('Invalid XML element: ' + str(arg) + "\n")
                        exit(XML_ERR)
                    arg = {
                        'type': arg.attrib['type'],
                        'value': arg.text
                    }
                    args.append(arg)
                    arg_counter += 1
                # Add instruction with args to array of all instructions
                allInst.addInst(child.attrib['opcode'].upper(), args)
                inst_order += 1

                if child.attrib['opcode'] == 'LABEL':   # If label, add to array of all labels
                    if args[0]['type'] != 'label' or len(args) != 1:
                        allInst.setCounter(inst_order - 1)
                        retError(TYPE_ERR, 'Invalid operand')
                    if args[0]['value'] in label_arr:  # Redefinition of label
                        allInst.setCounter(inst_order - 1)
                        retError(SEM_ERR, 'Redefinition of label')

                    allInst.setCounter(inst_order - 1)      # Only for error message
                    nameCheck(args[0]['value'])
                    allInst.setCounter(0)               # Get back
                    label_arr.update({args[0]['value']: int(child.attrib['order'], base=10)})

            elif child.tag != 'name' and child.tag != 'description':  # Unknown element
                sys.stderr.write('Unknown XML element:' + str(child))
                exit(XML_ERR)
    except SystemExit as exit_ex:
        exit(exit_ex.code)
    except FileNotFoundError:
        sys.stderr.write("Unable to open source file.\n")
        exit(IN_ERR)
    except Exception as ex:
        sys.stderr.write("Invalid XML, exception:\n" + str(ex) + "\n")
        exit(XML_ERR)


"""""""""""""""""""""""""""""""""""""""""""""""Global variables"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
label_arr = {}          # Array of all labels
allInst = InstArr()
global_frame = {}       # Array of variables in global frame
frame_stack = []        # Stack of frames
data_stack = []         # Stack of variables
call_stack = []         # Stack of call
tmp_frame = {'undefined': True}



def main():
    global tmp_frame
    # Parse arguments
    try:
        parser = argparse.ArgumentParser(description='Interpret of IPPcode18 XML representantion.')
        parser.add_argument('--source', metavar='source file', nargs=1, help='source XML file for interpretation')
        arguments = parser.parse_args()
    except SystemExit:                  # To return proper value
        exit(ARG_ERR)

    path = arguments.source[0]
    createArr(path)
    global allInst

    # Iterate in all instructions
    #print(allInst.instructions)
    while allInst.getCounter() < allInst.count:
        #print(tmp_frame)
        #print()
        #print(allInst.inst_counter)
        #print(allInst.instructions[allInst.inst_counter])
        #pprint(global_frame)
        allInst.incCounter()
        mainSwitch(allInst.getInst()['opcode'])
        #pprint(global_frame)
        #print('stack')
        #pprint(data_stack)


if __name__ == "__main__":
    main()
