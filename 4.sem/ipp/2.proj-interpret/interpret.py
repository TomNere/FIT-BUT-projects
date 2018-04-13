import argparse
import sys                  # Printing to STDERR
from pprint import pprint
import xml.etree.ElementTree as ET

"""""""""""""""""""""""""""""""""ERROR_CODES"""""""""""""""""""""""""""""""""""
XML_ERR = 31
SYN_ERR = 32
ARG_ERR = 10
IN_ERR = 11
OUT_ERR = 12
SEM_ERR = 52
TYPE_ERR = 53
VAR_ERR = 54
FRAME_ERR = 55
VOID_ERROR = 56
ZERO_ERR = 57
STR_ERR = 58
OTHER_ERR = 99

ARG_STR = "Invalid argument(s)"

"""""""""""""""""""""""""""""""""""""HELPING_FUNCTIONS"""""""""""""""""""""""""""""""""""""""""""


def retError(err, msg):
    sys.stderr.write("Error found in instruction number " + str(allInst.inst_counter) + ".\n")
    sys.stderr.write(msg + ".\n")
    exit(err)


def getFrame(frame):
    global global_frame, tmp_frame, frame_stack

    if frame == 'GF':
        return global_frame
    elif frame == 'LF':
        if not frame_stack:
            retError(FRAME_ERR, 'Undefined local frame')
        return frame_stack[-1]
    elif frame == 'TF':
        if tmp_frame['undefined'] is True:
            retError(FRAME_ERR, 'Undefined temporary frame')
        return tmp_frame
    else:
        retError(SYN_ERR, 'Invalid variable name')


# Translate variable if variable, else return constant
def varTranslate(var):
    if var['type'] == 'bool' or var['type'] == 'int' or var['type'] == 'string':
        new_var = dict(var)
        typeCheck(new_var)
        return new_var
    elif var['type'] == 'var':
        frame = getFrame(var['value'][:2])
        if var['value'][3:] in frame:
            return frame[var['value'][3:]]
        else:
            retError(VAR_ERR, "Undefined variable")
    else:
        retError(SYN_ERR, "Unknown type of symbol")


# Check number of arguments
def argCheck(inst, number):
    if len(inst['args']) != number:
        retError(ARG_ERR, ARG_STR)


def assignValue(var, new_value):
    frame = getFrame(var['value'][:2])
    if var['value'][3:] in frame:
        frame.update({var['value'][3:]: new_value})


def typeCheck(var):
    if var['type'] == 'int':
        number = intCheck(var['value'])
        if number is False:
            retError(SEM_ERR, "Invalid integer value")
        var.update({'value': number})
    elif var['type'] == 'bool':
        if var['value'] == 'true':
            var.update({'value': True})
        elif var['value'] == 'false':
            var.update({'value': False})
        else:
            retError(SEM_ERR, "Invalid bool value")
    elif var['type'] == 'string':
        var.update({'value': strCheck(var['value'])})
    elif var['type'] == 'label':
        pass
    else:
        retError(SYN_ERR, 'Unknown variable type')


def tryInput():
    try:
        var = input()
    except EOFError:
        var = False
    return var


def intCheck(var):
    try:
        integer = int(var, base=10)
    except ValueError:
        integer = False
    return integer


def strCheck(var):
    if var is None:     # Empty string
        return ""

    new_string = ""
    counter = 0
    while counter < len(var):
        if var[counter] == '\\':
            number = var[counter + 1:counter + 4]
            if number.isdigit():
                new_string += chr(int(number, base=10))
                counter += 3
            else:
                retError(SEM_ERR, 'Bad escape sequence in string')
        elif var[counter].isspace():
            retError(SEM_ERR, 'Unknown character in string')
        else:
            new_string += var[counter]
        counter += 1
    return new_string


"""""""""""""""""""""""""""""""""""""INSTRUCTIONS"""""""""""""""""""""""""""""""""""""""""""


# Nothing to do
def inothing(inst):
    pass


def iframes(inst):
    argCheck(inst, 0)
    global tmp_frame
    global frame_stack

    if inst['opcode'] == 'CREATEFRAME':
        tmp_frame.clear()
        tmp_frame = {'undefined': False}

    elif inst['opcode'] == 'PUSHFRAME':
        if tmp_frame['undefined'] is True:  # TF doesn't exist
            retError(FRAME_ERR, 'Undefined temporary frame')
        frame_stack.append(dict(tmp_frame))
        tmp_frame.clear()
        tmp_frame = {'undefined': True}

    elif inst['opcode'] == 'POPFRAME':
        if not frame_stack:                 # Empty stack of frames
            retError(FRAME_ERR, 'Stack of frames is empty')
        if tmp_frame['undefined'] is True:  # TF doesn't exist
            retError(FRAME_ERR, 'Undefined temporary frame')

        tmp_frame = dict(frame_stack[-1])
        del frame_stack[-1]


def ireturn(inst):
    argCheck(inst, 0)
    global call_stack
    if not call_stack:      # Empty stack of calls
        retError(VOID_ERROR, 'Empty call stack')

    #print("callstack:"+str(call_stack[-1]))
    allInst.setCounter(call_stack[-1])
    del call_stack[-1]


def ibreak(inst):
    argCheck(inst, 0)
    global global_frame, tmp_frame, frame_stack
    sys.stderr.write('Global frame: ' + global_frame + "\n" +
                     'Local frame: ' + frame_stack[-1] + "\n" +
                     'Tmp frame: ' + tmp_frame)


def idefvar(inst):
    argCheck(inst, 1)
    name = inst['args'][0]['value']
    frame = getFrame(name[:2])
    name = name[3:]
    frame.update({name: {'type': None, 'value': None}})


def icall(inst):
    argCheck(inst, 1)
    global call_stack, allInst
    call_stack.append(int(allInst.inst_counter))

    if inst['args'][0]['type'] == 'label':
        if inst['args'][0]['value'] in label_arr:
            allInst.setCounter(label_arr[inst['args'][0]['value']])
        else:
            retError(SEM_ERR, 'Undefined label')
    else:
        retError(TYPE_ERR, 'Label expected.')


# Push value on the top of data stack
def ipushs(inst):
    argCheck(inst, 1)
    var = varTranslate(inst['args'][0])
    global data_stack
    data_stack.append(dict(var))


def ipops(inst):
    argCheck(inst, 1)
    global data_stack
    assignValue(inst['args'][0], data_stack[-1])
    del data_stack[-1]


def iwrite(inst):
    argCheck(inst, 1)
    var = varTranslate(inst['args'][0])
    if var['type'] == 'int':                # Int to str
        printed = str(var['value'])
    elif var['type'] == 'bool':             # Bool values translate
        if var['value'] is True:
            printed = 'true'
        else:
            printed = 'false'
    else:
        printed = var['value']

    if inst['opcode'] == 'WRITE':
        print(printed)
    else:                           # Dprint
        sys.stderr.write(printed)


def ijump(inst):
    argCheck(inst, 1)
    global label_arr, allInst
    if inst['args'][0]['value'] in label_arr:
        allInst.setCounter(label_arr[inst['args'][0]['value']])
    else:
        retError(SEM_ERR, 'Undefined label')


def imove(inst):
    var = varTranslate(inst['args'][1])
    #print('iMove')
    #print(var)
    #print(tmp_frame)
    assignValue(inst['args'][0], var)
    #print(tmp_frame)
    #print('--------------')


def iint2char(inst):
    argCheck(inst, 2)
    var = varTranslate(inst['args'][1])
    if var['type'] != 'int':
        retError(SEM_ERR, 'Integer expected')

    char = chr(var['value'])
    if char is ValueError:                          # Unable to convert
        retError(STR_ERR, "Invalid Unicode value")
    assignValue(inst['args'][0], {'type': 'string', 'value': char})


def iread(inst):
    argCheck(inst, 2)
    if inst['args'][1]['type'] != 'type':
        retError(TYPE_ERR, 'Type expected')

    var = tryInput()
    if inst['args'][1]['value'] == 'bool':
        if var == 'true':
            var = True
        else:
            var = False
    elif inst['args'][1]['value'] == 'string':
        if var is False:
            var = ''
    elif inst['args'][1]['value'] == 'int':
        if var is False:
            var = 0
        else:
            var = intCheck(var)
            if var is False:
                var = 0
    else:
        retError(SYN_ERR, 'Unknown type')

    assignValue(inst['args'][0], {'type': inst['args'][1]['value'], 'value': var})


def istrlen(inst):
    argCheck(inst, 2)
    string = varTranslate(inst['args'][1])
    if string['type'] != 'string':
        retError(TYPE_ERR, 'String expected')
    length = len(string['value'])
    assignValue(inst['args'][0], {'type': 'int', 'value': length})


def itype(inst):
    argCheck(inst, 2)
    frame = getFrame(inst['args'][1]['value'][:2])
    if (frame is not False) and (inst['args'][1]['value'][3:] not in frame):
        typee = ''
    else:
        var = varTranslate(inst['args'][1])
        typee = var['type']
    assignValue(inst['args'][1], {'type': 'string', 'value': typee})


def ievaluation(inst):
    argCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])
    number = 0

    if op1['type'] == 'int' and op2['type'] == 'int':
        if inst['opcode'] == 'ADD':
            number = op1['value'] + op2['value']
        elif inst['opcode'] == 'SUB':
            number = op1['value'] - op2['value']
        elif inst['opcode'] == 'MUL':
            number = op1['value'] * op2['value']
        elif inst['opcode'] == 'IDIV':
            if op2['value'] != 0:
                number = op1['value'] // op2['value']
            else:
                retError(ZERO_ERR, 'Division by Zero')
        assignValue(inst['args'][0], {'type': 'int', 'value': number})
    else:
        retError(TYPE_ERR, 'Integers expected')


def icomparison(inst):
    argCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])
    flag = False

    if (op1['type'] == 'int' and op2['type'] == 'int') or (op1['type'] == 'string' and op2['type'] == 'string'):
        if inst['opcode'] == 'LT':
            flag =  op1['value'] < op2['value']
        elif inst['opcode'] == 'GT':
            flag = op1['value'] > op2['value']
        elif inst['opcode'] == 'EQ':
            flag = op1['value'] == op2['value']

    elif op1['type'] == 'bool' and op2['type'] == 'bool':
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
    argCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])
    flag = False

    if op1['type'] == 'bool' and op2['type'] == 'bool':
        if inst['opcode'] == 'AND':
            flag = op1['value'] and op2['value']
        elif inst['opcode'] == 'OR':
            flag = op2['value'] or op2['value']
    else:
        retError(TYPE_ERR, 'Bool operands expected')

    assignValue(inst['args'][0], {'type': 'bool', 'value': flag})


def inot(inst):
    argCheck(inst, 2)
    op1 = varTranslate(inst['args'][1])

    if op1['type'] == 'bool':
        assignValue(inst['args'][0], {'type': 'bool', 'value': not op1['value']})
    else:
        retError(TYPE_ERR, 'Bool operand expected')


def istri2int(inst):
    argCheck(inst, 3)
    string = varTranslate(inst['args'][1])
    pos = varTranslate(inst['args'][2])

    if string['type'] == 'string' and pos['type'] == 'int':
        if (pos['value'] < 0) or (pos['value'] > len(string['value']) - 1):         # Check for valid index
            retError(STR_ERR, 'Index out of bounds')
        if inst['opcode'] == 'STRI2INT':
            var = ord(string['value'][pos['value']])
            assignValue(inst['args'][0], {'type': 'int', 'value': var})
        elif inst['opcode'] == 'GETCHAR':
            var = string['value'][pos['value']]
            assignValue(inst['args'][0], {'type': 'string', 'value': var})
    else:
        retError(TYPE_ERR, 'Invalid type of operand(s)')


def iconcat(inst):
    argCheck(inst, 3)
    op1 = varTranslate(inst['args'][1])
    op2 = varTranslate(inst['args'][2])

    if op1['type'] == 'string' and op2['type'] == 'string':
        string = op1['value'] + op2['value']
        assignValue(inst['args'][0], {'type': 'string', 'value': string})
    else:
        retError(TYPE_ERR, 'String operands expected')


def isetchar(inst):
    argCheck(inst, 3)
    pos = varTranslate(inst['args'][1])
    char = varTranslate(inst['args'][2])
    var = False

    if inst['args'][0]['type'] == 'var':
        var = varTranslate(inst['args'][0])
    else:
        retError(TYPE_ERR, 'Variable expected')

    if var['type'] == 'string' and pos['type'] == 'int' and char['type'] == 'string':
        # Check for valid index
        if (pos['value'] < 0) or (pos['value'] > len(var['value']) - 1) or (len(char['value']) == 0):
            retError(STR_ERR, 'Index out of bounds')

        var['value'] = var['value'][:pos['value']] + char['value'][0] + var['value'][pos['value'] + 1:]
        assignValue(inst['args'][0], var)
    else:
        retError(TYPE_ERR, 'Invalid type of argument(s)')


def ijumpif(inst):
    argCheck(inst, 3)
    if len(inst['args']) != 3:
        retError(SYN_ERR, ARG_STR)

    var1 = varTranslate(inst['args'][1])
    var2 = varTranslate(inst['args'][2])

    if var1['type'] != var2['type']:
        retError(TYPE_ERR, "Type incompatibility")

    if inst['args'][0]['value'] in label_arr:
        if inst['opcode'] == 'JUMPIFEQ':                                    # JUMPIFEQ
            if var1 == var2:
                allInst.setCounter(label_arr[inst['args'][0]['value']])
        else:                                                               # JUMPIFNEQ
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

    func = switcher.get(inst, lambda: "Unknown")
    func(allInst.getInst())


""" Class representing array of all instructions
#   Include program counter
"""
class InstArr:

    def __init__(self):
        self.count = 0
        self.instructions = []
        self.inst_counter = 0
        self.count_stack = []

    def addInst(self, opcode, args):
        instruction = {
            'opcode': opcode,
            'args': args
        }
        self.instructions.append(instruction)
        self.count += 1

    def getInst(self):
        return self.instructions[self.inst_counter - 1]

    def incCounter(self):
        self.inst_counter += 1

    def setCounter(self, number):
        self.inst_counter = number

    def stackAdd(self, number):
        self.count_stack.append(number)

    def show(self):
        pprint(self.instructions)


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
                        sys.stderr.write('Invalid XML element:' + str(arg) + "\n")
                        exit(XML_ERR)
                    arg = {
                        'type': arg.attrib['type'],
                        'value': arg.text
                    }
                    args.append(arg)
                    arg_counter += 1
                # Add instruction with args to array of all instructions
                allInst.addInst(child.attrib['opcode'], args)
                inst_order += 1

                if child.attrib['opcode'] == 'LABEL':   # If label, add to array of all labels
                    if args[0]['type'] != 'label' or len(args) != 1:
                        retError(SYN_ERR, ARG_STR)

                    if args[0]['value'] in label_arr:  # Redefinition of label
                        retError(SEM_ERR, "Redefinition of label")

                    label_arr.update({args[0]['value']: int(child.attrib['order'], base=10)})

            elif child.tag != 'name' and child.tag != 'description':  # Unknown element
                sys.stderr.write('Unknown XML element:' + str(child))
                exit(XML_ERR)
    except SystemExit as exit_ex:
        sys.exit(exit_ex.code)
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
    parser = argparse.ArgumentParser(description='Something')
    parser.add_argument('--source', metavar='--source', nargs=1, help='Source file')
    arguments = parser.parse_args()

    path = arguments.source[0]

    global allInst
    createArr(path)

    # Iterate in all instructions
    #print(allInst.instructions)
    while allInst.inst_counter < allInst.count:
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
