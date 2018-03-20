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

ARG_STR = "Invalid arguments"

def retError(err, msg):
    sys.stderr.write("Error found in instruction number " + str(allInst.inst_counter) + ".\n")
    sys.stderr.write(msg + ".\n")
    exit(err)

def getFrame(frame):
    if frame == 'GF':
        return global_frame
    elif frame == 'LF':
        pass
    elif frame == 'TF':
        pass

# Translate variable if variable, else return constant
def varTranslate(var):
    if var['type'] == 'bool' or var['type'] == 'int' or var['type'] == 'string':
        return var
    elif var['type'] == 'var':
        frame = getFrame(var['text'][:2])
        if var['text'][3:] in frame:
            return frame[var['text'][3:]]
        else:
            retError(VAR_ERR, "Undefined variable")
    else:
        retError(SYN_ERR, "Unknown type of symbol")

"""""""""""""""""""""""""""""""""""""HELPING_FUNCTIONS"""""""""""""""""""""""""""""""""""""""""""

def icreateframe():
    return ""

def ipushframe():
    return ""

def ipopframe():
    return ""

def ireturn():
    return ""

def ibreak():
    return ""

def idefvar(inst):
    name = inst['args'][0]['text']
    frame = getFrame(name[:2])
    name = name[3:]
    frame.update({name: {'type': None, 'value': None}})

def icall():
    return ""

def ipushs():
    return ""

def ipops():
    return ""

def iwrite():
    return ""

def ilabel(inst):
    return

def ijump():
    return ""

def idprint():
    return ""

def imove(inst):
    if inst['args'][1]['type'] != 'var':        # Constant
        frame = getFrame(inst['args'][0]['text'][:2])
        if inst['args'][0]['text'][3:] in frame:
            frame.update({inst['args'][0]['text'][3:]: {'type': inst['args'][1]['type'], 'value': inst['args'][1]['text']}})
        else:
            exit(31)

def iint2char():
    return ""

def iread():
    return ""

def istrlen():
    return ""

def itype():
    return ""

def iadd():
    return ""

def isub():
    return ""

def imul():
    return ""

def iidiv():
    return ""

def ilt():
    return ""

def igt():
    return ""

def ieq():
    return ""

def iand():
    return ""

def ior():
    return ""

def inot():
    return ""

def istri2int():
    return ""

def iconcat():
    return ""

def igetchar():
    return ""

def isetchar():
    return ""

def ijumpifeq(inst):
    if len(inst['args']) != 3:
        retError(SYN_ERR, ARG_STR)

    var1 = varTranslate(inst['args'][1])
    var2 = varTranslate(inst['args'][2])

    if var1['type'] != var2['type']:
        retError(TYPE_ERR, "Type incompatibility")

    if inst['args'][0]['text'] in label_arr:
        if var1 == var2:                                                # Jump
            allInst.setCounter(label_arr[inst['args'][0]['text']])
        else:                                                           # Nothing to do
            pass
    else:
        retError(SEM_ERR, "Undefined label")

def ijumpifneq():
    return ""

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


def createArr(arr, labels, path):
    tree = ET.parse(path)
    root = tree.getroot()

    for child in root:
        if child.tag == 'instruction':              # If instruction, add to array
            args = []

            for arg in child:                   # Iterate in arguments
                arg = {
                    'type': arg.attrib['type'],
                    'text': arg.text
                }
                args.append(arg)
            arr.addInst(child.attrib['opcode'], args)       # Add instruction with args to array of all instructions

            if child.attrib['opcode'] == 'LABEL':   # If label, add to array of all labels
                if args[0]['type'] != 'label' or len(args) != 1:
                    retError(SYN_ERR, ARG_STR)

                if args[0]['text'] in label_arr:  # Redefinition of label
                    retError(SEM_ERR, "Redefinition of label.")

                label_arr.update({args[0]['text']: child.attrib['order']})

        elif child.tag != 'name' and child.tag != 'description':  # Unknown element
            exit(31)


"""""""""""""""""""""""""""""""""""""""""""""""Global variables"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
label_arr = {}          # Array of all labels
allInst = InstArr()
global_frame = {}       # Array of variables in global frame
frame_stack = {}        # Stack of frames


def main():

    # Parse arguments
    parser = argparse.ArgumentParser(description='Something')
    parser.add_argument('--source', metavar='--source', nargs=1, help='Source file')
    arguments = parser.parse_args()

    path = arguments.source[0]

    global label_arr
    global allInst
    createArr(allInst, label_arr, path)

    # Iterate in all instructions
    while allInst.inst_counter < allInst.count:
        allInst.incCounter()
        globals()['i' + allInst.getInst()['opcode'].lower()](allInst.getInst())         # Call function assigned to opcode


if __name__ == "__main__":
    main()
