<?php 
/*********************CLASS DEFS***************************/

/**
* This class represents instruction
*/
/*
class Instruction {
    public $name;
    public $params;
}
*/

const ARG_ERR = 10;

/********************ARRAYS FOR INSTRUCTIONS*****************/
/**
* All instructions 
* KEY - operation code
* VALUE - array of parameters
* 1 - variable
* 2 - label
* 3 - symb
* 4 - type
*/
$all_inst = array(
    "createframe" => array (), 
    "pushframe" => array (), 
    "popframe" => array (), 
    "return" => array (), 
    "break" => array (),

    "defvar" => array (1), 
    "call" => array (2), 
    "pushs" => array (3), 
    "pops" => array (1), 
    "write" => array (3), 
    "label" => array (2), 
    "jump" => array (2), 
    "dprint" => array (3),

    "move" => array (1, 3), 
    "int2char" => array (1, 3), 
    "read" => array (1, 4), 
    "strlen" => array (1, 3), 
    "type" => array (1, 3),

    "add" => array(1, 3, 3), 
    "sub" => array(1, 3, 3), 
    "mul" => array(1, 3, 3), 
    "idiv" => array(1, 3, 3), 
    "lt" => array(1, 3, 3), 
    "gt" => array(1, 3, 3), 
    "eq" => array(1, 3, 3), 
    "and" => array(1, 3, 3), 
    "or" => array(1, 3, 3), 
    "not" => array(1, 3, 3), 
    "stri2int" => array(1, 3, 3), 
    "concat" => array(1, 3, 3), 
    "getchar" => array(1, 3, 3), 
    "setchar" => array(1, 3, 3), 
    "jumpifeq" => array(2, 3, 3), 
    "jumpifneq" => array(2, 3, 3)
);

/*********************FUNCTION DEFS************************/

// Skip white characters
function skipWhite() {
    $char;
    while (true) {
        if (feof(STDIN)) {
            return -1;
        }

        $char = fgetc(STDIN);

        if ($char == '#') {
            fgets(STDIN);
            continue;
        }
        if ($char == '\n') {
            return '\n';
        }
        if (ctype_space($char)) {
            continue;
        }
        else {
            return $char;
        }
    }
}

// Read Escape sequence
function getEscape($end) {

    $number = fgets(STDIN, 3);
    // 000-999
    if (ctype_digit($number)) {
        if (strcmp($number, "060") == 0)
            return "&lt";
        else if (strcmp($number, "062") == 0)
            return "&gt";
        else if (strcmp($number, "038"))
            return "&amp";
        else 
            return $number;
    }
    else
        return "error";
}

// Read string
function getString($end) {
    $c;
    $str = "";
    while($c = fgetc(STDIN)) {
        if (($c == '\n' && $end) || $c == ' ' || $c == '\t') {
            break;
        }
        if ($c == 92) {
            $tmp = getEscape($end);

            if (strcmp($tmp, "error") != 0) {
                $str.$tmp;
                break;
            }
            else
                return "error";
        }
        // Comment
        if ($c == 35) {
            skipComment();
            break;
        }
        if (ctype_print($c)) {
            $str.=$c;
            continue;
        }
    }
    return $str;
}


// Read int constant
function getInt($end) {
    $c = fgetc(STDIN);
    $number;

    if (is_numeric($c) || $c == '+' || $c == '-') {
        $number = $c;
    }
    else 
        return "error";

    while ($c = fgetc(STDIN)) {
        if (is_numeric($c)) {
            $number.=$c;
        }
        else 
            break;
    }
    if (($c == '\n' && $end) || $c == '\t' || $c == ' ') {
        return $number;
    }
    else
        return "error";
}

// Constant or variable
function getSymb($end, &$type) {
    $str = skipWhite();

    if ($str == 'L' || $str == 'T' || $str == 'G') {
        $tmp = fgetc(STDIN);
        if ($tmp == 'F') {
            $str.=$tmp;
        }
        else {
            return "error";
        }

        if (($tmp = fgetc(STDIN)) != '@') {
            return "error";
        }
        else {
            $str.="@";
            $tmp = getVarLab($end, false);
            $type = 1;
            return $str.$tmp;         
        }
    }
    
    if (ctype_lower($str)) {
        $tmp;
        while ($tmp = fgetc(STDIN)) {
            if (ctype_lower($tmp)) {
                $str.$tmp;
            }
            else {
                break;
            }
        }
        if (strcmp($str, "int") == 0 && $tmp == '@') {
            $tmp = getInt($end);
            if (strcmp($tmp, "error") != 0) {
                $str.="@";
                $type = 2;
                return $str.$tmp;
            } 
        }
        if (strcmp($str, "string") == 0 && $tmp == '@') {
            $type = 3;
            return getString();
        }
        if (strcmp($str, "bool") == 0 && $tmp == '@')
            $str.='@';
            $tmp = "";
            while ($c = fgetc(STDIN)) {
                if (ctype_lower($c)) {
                    $tmp.$c;
                }
                else {
                    break;
                }
            }
            if (strcmp($tmp, "true") == 0 || strcmp($tmp, "false") == 0) {
                $type = 4;
                return $str.$tmp;
            }
        }
    }

    // @ check
    

//}

// First part of variable name
function getFrame() {
    $str = skipWhite();

    // LF, TF or GF
    if ($str == 'L' || $str == 'T' || $str == 'G') {
        $tmp = fgetc(STDIN);
        if ($tmp == 'F') {
            $str.=$tmp;
        }
        else {
            return "error";
        }
    }
    else {
        return "error";
    }

    // @ check
    if (($tmp = fgetc(STDIN)) != '@') {
        return "error";
    }
    return $str."@";
}

// Variable or label
function getVarLab($end, $is_var) {
    if ($is_var == true)
        $str = getFrame();
    else 
        $str = "";

    if (strcmp($str, "error") == 0) {
        return "error";
    }

    // Must start with alpha or special
    $tmp = fgetc(STDIN);
    if (ctype_alpha($tmp) || $tmp == '_' || $tmp == '-' || $tmp == '$' || $tmp == '&' || $tmp == '%' || $tmp == '*') {
        $str.=$tmp;
    }
    else {
        return "error";
    }

    while (true) {
        $tmp = fgetc(STDIN);

        if (ctype_alnum($tmp) || $tmp == '_' || $tmp == '-' || $tmp == '$' || $tmp == '&' || $tmp == '%' || $tmp == '*') {
            $str.=$tmp;
            continue;
        }
        if ($tmp == ' ' || $tmp == '\t') {
            break;
        }
        if ($tmp == '\n' && $end == true) {
            break;
        }
        else {
            return "error";
        }
    }
    return $str;
}


function getTyp($end) {
    $str = skipWhite();
    while (true) {
        $c = fgetc(STDIN);

        if (ctype_alpha($c)) {
            $str.=$c;
            continue;    
        }
        if ($c == ' ' || $c == '\t') {
            break;
        }
        if ($c == '\n' && $end == true) {
            break;
        }
        else {
            return "error";
        }
    }
    if (strcmp($str, "int") == 0 || strcmp($str, "string") == 0 || strcmp($str, "bool") == 0) {
        return $str;
    }
    else {
        return "error";
    }
}

function getInst() {
    $op_code = skipWhite();

    while ($op_code == '\n')
        $op_code = skipWhite();

    if ($op_code == -1)
        return -1;

    //global $help_arr, $inst_0, $inst_1, $inst_2, $inst_3;
    global $all_inst;

    $no_arg = false;
    //$inst = new Instruction();
    $c;
    while ($c = fgetc(STDIN)) {
        if ($c == ' ' || $c == '\t')
            break;
        if (ctype_alnum($c)) {
            $op_code.=$c;
        }
        else if ($c == '\n') {
            $no_arg = true;
        }
        else {
            return -2;
        }
    }
    // Case insentive
    $op_code = strtolower($op_code);

    foreach ($all_inst as $key => $value) {
        //print $key;
        //print $op_code;
        if (strcmp($key, $op_code) == 0) {
            // \n after operation code
            if ($no_arg == true) {
                if (count($all_inst[$key]) != 0)
                    return -2;
            }
            return $op_code;
        }
    }
    // Unknown operation code
    return -2;
}

function main() {

    // Arguments check
    global $argc;
    global $argv;
    
    if ($argc > 1) {
        if ($argv[1] == '--help') {
            print "show passed\n";
            return 0;
        }
        else {
            fwrite(STDERR, "Unknown arguments!\n");
            return 10;
        }
    }
    
    // First line check
    $char = skipWhite();
    $first_line = $char.rtrim(strtolower(fgets(STDIN)));
    if (strcmp($first_line, ".ippcode18") != 0) {
        fwrite(STDERR, ".IPPcode18 is missing!\n");
        return 21;
    }

    // Basic XML string
$xml_str = <<<XML
<?xml version="1.0" encoding="UTF-8"?>
<program language="IPPcode18">
</program>
XML;
    
    $xml_el = new SimpleXMLElement($xml_str);

    // Instruction counter
    $inst_count = 0;

    // Repeat until error or EOF
    while (true) {
        $var = getInst();
        // End of file
        if ($var == -1) {
            break;
        }
        // Unknown instruction
        if ($var == -2) {
            return 21;
        }

        // $end signalize last parameter
        $end;
        $inst_count++;

        global $all_inst;

        $xml_inst = $xml_el->addChild("instruction");
        $xml_inst->addAttribute("order", $inst_count);
        $xml_inst->addAttribute("opcode", strtoupper($var));  

        foreach ($all_inst[$var] as $key => $value) {
            if (count($all_inst[$var]) == ($key + 1)) 
                $end = true;
            else
                $end = false;

            $xml_arg = $xml_inst->addChild("arg".strval($key + 1));

            switch ($value) {
                case 1:
                    $str = getVarLab($end, true);
                    $xml_arg->addAttribute("type", "var");
                    $xml_arg = $str;
                    break;
                case 2:
                    $str = getVarLab($end, false);
                    $xml_arg->addAttribute("type", "label");
                    $xml_arg = $str;
                    break;
                case 3:
                    //$c = skipWhite();
                    //if ()
                    $type;
                    $str = getSymb($end, $type);
                    switch ($type) {
                        case 1:
                            $xml_arg->addAttribute("type", "var");
                            break;
                        case 2:
                            $xml_arg->addAttribute("type", "int");
                            break;
                        case 3:
                            $xml_arg->addAttribute("type", "string");
                            break;
                        case 4:
                            $xml_arg->addAttribute("type", "bool");
                            break;
                        default:
                            // WTF
                            break;
                    }
                    $xml_arg = $str;
                    break;
                case 4:
                    $str = getTyp($end);
                    $xml_arg->addAttribute("type", "type");
                    $xml_arg = $str;
                    break;
                default:
                    // WTF
                    break;
            }
        }
    }
    
    echo $xml_el->asXml();
    return 0;
}

return main();

?>