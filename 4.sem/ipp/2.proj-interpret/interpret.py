import argparse
from pprint import pprint
import xml.etree.ElementTree as ET

def iCreateFrame():
    return ""

def iPushFrame():
    return ""

def iPopFrame():
    return ""

def iReturn():
    return ""

def iBreak():
    return ""

def iDefVar():
    inst = allInst.getInst()
    name = inst['args'][0]['text']
    print(name)


def iCall():
    return ""

def iPushs():
    return ""

def iPops():
    return ""

def iWrite():
    return ""

def iLabel():
    return ""

def iLabel():
    return ""

def iJump():
    return ""

def iDPrint():
    return ""

def iMove():
    return ""

def iInt2char():
    return ""

def iRead():
    return ""

def iStrLen():
    return ""

def iType():
    return ""

def iAdd():
    return ""

def iSub():
    return ""

def iMul():
    return ""

def iIDiv():
    return ""

def iLt():
    return ""

def iGt():
    return ""

def iEq():
    return ""

def iAnd():
    return ""

def iOr():
    return ""

def iNot():
    return ""

def iStri2int():
    return ""

def iConcat():
    return ""

def iGetChar():
    return ""

def iSetChar():
    return ""

def iJumpIfEq():
    return ""

def iJumpIfNEq():
    return ""

def mainSwitch(inst):
    switcher = {
        'CREATEFRAME': iCreateFrame,
        'PUSHFRAME': iPushFrame,
        'POPFRAME': iPopFrame,
        'RETURN': iReturn,
        'BREAK': iBreak,
        'DEFVAR': iDefVar,
        'CALL': iCall,
        'PUSHS': iPushs,
        'POPS': iPops,
        'WRITE': iWrite,
        'LABEL': iLabel,
        'JUMP': iJump,
        'DPRINT': iDPrint,
        'MOVE': iMove,
        'INT2CHAR': iInt2char,
        'READ': iRead,
        'STRLEN': iStrLen,
        'TYPE': iType,
        'ADD': iAdd,
        'SUB': iSub,
        'MUL': iMul,
        'IDIV': iIDiv,
        'LT': iLt,
        'GT': iGt,
        'EQ': iEq,
        'AND': iAnd,
        'OR': iOr,
        'NOT': iNot,
        'STRI2INT': iStri2int,
        'CONCAT': iConcat,
        'GETCHAR': iGetChar,
        'SETCHAR': iSetChar,
        'JUMPIFEQ': iJumpIfEq,
        'JUMPIFNEQ': iJumpIfNEq,
    }

    func = switcher.get(inst, lambda: "Unknown")
    func()

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

    #def doStuff(self):
    #    mainSwitch(self.instructions[self.inst_counter])

    def show(self):
        pprint(self.instructions)


def createArr(arr, labels, path):
    tree = ET.parse(path)
    root = tree.getroot()

    for child in root:
        if child.tag == 'instruction':              # If instruction, add to array
            args = []
            if child.attrib['opcode'] == 'LABEL':   # If label, add to array of all labels
                label = {
                    'name': child.text,
                    'pos': child.attrib['order']
                }
                labels.append(label)
            else:
                for arg in child:                   # Iterate in arguments
                    arg = {
                        'type': arg.attrib['type'],
                        'text': arg.text
                    }
                    args.append(arg)
            arr.addInst(child.attrib['opcode'], args)       # Add instruction with args to array of all instructions
        elif child.tag != 'name' and child.tag != 'description':  # Unknown element
            exit(31)


"""""""""""""""""""""""""""""""""""""""""""""""Global variables"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
label_arr = []          # Array of all labels
allInst = InstArr()
global_frame = []       # Array of variables in global frame


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
    while True:
        allInst.incCounter()
        mainSwitch(allInst.getInst()['opcode'])

        if allInst.inst_counter == allInst.count:
            break
            allInst

"""
    arg = []
    arg.append('prvy')
    arg.append('druhy')

    allInst = InstArr()
    allInst.addInst('STRI2INT', arg)
    allInst.addInst('asd', arg)
    allInst.show()
"""
if __name__ == "__main__":
    main()