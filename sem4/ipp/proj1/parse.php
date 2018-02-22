<?php
/*******************CONSTANTS DEFINITIONS********************/
const ARG_ERR = 10;
const EOF = -1;
const ERR = -2;

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

function terminate($code, $str) {
    fwrite(STDERR, $str."\n");
    exit($code);
}

// Skip white characters
function skipWhite() {
    $char;
    while (true) {
        if (feof(STDIN))
            return EOF;

        $char = fgetc(STDIN);

        if (strcmp($char, "#") == 0) {
            fgets(STDIN);
            continue;
        }
        if (strcmp($char, "\n") == 0)
            return "\n";

        if (ctype_space($char))
            continue;
        else
            return $char;
    }
}

// Read Escape sequence
function getEscape() {
    $number = fgets(STDIN, 3);
    // 000-999
    if (ctype_digit($number)) {
        if (strcmp($number, "060") == 0)
            return "&lt";
        else if (strcmp($number, "062") == 0)
            return "&gt";
        else if (strcmp($number, "038") == 0)
            return "&amp";
        else 
            return $number;
    }
    else
        terminate(21, "Escape sequence expected.")
}

// Read string
function getString($end) {
    $c;
    $str = "";

    while($c = fgetc(STDIN)) {
        if (((strcmp($c, "\n") == 0) && $end) || (strcmp($c, " ") == 0) || (strcmp($c, "\t") == 0))
            break;
        if (strcmp($c, chr(92)) == 0) {
            $tmp = getEscape();
            $str.=chr(92);
            $str.=$tmp;
            continue;
        }
        // Comment
        if (strcmp($c, chr(35)) == 0) {
            fgets(STDIN);
            break;
        }
        if (ctype_print($c))
            $str.=$c;
    }
    return $str;
}


// Read int constant
function getInt($end) {
    $c = fgetc(STDIN);
    $number;

    if (is_numeric($c) || strcmp($c, "+") == 0 || strcmp($c, "-") == 0)
        $number = $c;
    else
        terminate(21, "Invalid int.");

    while ($c = fgetc(STDIN)) {
        if (is_numeric($c))
            $number.=$c;
        else
            break;
    }

    if ((strcmp($c, "\n") == 0 && $end) || strcmp($c, "\t") == 0 || strcmp($c, " ") == 0)
        return $number;
    else
        terminate(21, "Invalid int.");
}

// Constant or variable
function getSymb($end, &$type) {
    $frame
    $type = getFrame($frame, false);

    switch ($type) {
        case 1:
            return $frame.getVarLab($end, false);
            break;
        case 2:
            return getInt($end);
            break;
        case 3:
            return getString($end);
            break;
        case 4:
            $tmp = "";
            $c;
            while ($c = fgetc(STDIN)) {
                if (ctype_lower($c))
                    $tmp.=$c;
                else
                    break;
            }
            if (strcmp($tmp, "true") == 0 || strcmp($tmp, "false") == 0)
                return $tmp;
            break;
        
        default:
            terminate(21, "Unknown error.")
            break;
    }
}

/* Get frame or type - int, string, bool
* return int which symbolize
* 1 - frame
* 2 - int
* 3 - string
* 4 - bool
* arg $str is used when returning 1
*/
function getFrame(&$frame, $is_type) {
    $frame = skipWhite();

    if (strcmp($frame, "L") == 0 || strcmp($frame, "T") == 0 || strcmp($frame, "G") == 0) {

        if (strcmp(fgetc(STDIN), "F") == 0)
            $frame.="F";
        else
            terminate(21, "Invalid frame name.");

        if (strcmp(fgetc(STDIN), "@") != 0)
            terminate(21, "@ expected.");
        else {
            $frame."@";
            return 1;
        }
    }
    
    if (ctype_lower($frame)) {
        $c;
        while ($c = fgetc(STDIN)) {
            if (ctype_lower($c))
                $frame.=$c;
            else
                break;
        }

        if (strcmp($c, "@") != 0 && $is_type == false)
            terminate(21, "@ expected.");

        if (strcmp($frame, "int") == 0)
            return 2;
        if (strcmp($frame, "string") == 0)
            return 3;
        if (strcmp($frame, "bool") == 0)
            return 4;
    }
    terminate(21, "Unknown type of symbol.");
}

// Variable or label
function getVarLab($end, $is_var) {
    if ($is_var == true) {
        $frame;
        $is_frame = getFrame($frame, false);   
    }
    else
        $str = "";

    if ($is_frame != 1)
        terminate(21, "Variable name expected.");

    // Must start with alpha or special
    $c = fgetc(STDIN);
    if (ctype_alpha($c) || preg_match("/[_|-|&|$|%|*]/", $c) == 1)
        $str.=$c;
    else
        terminate(21, "Wrong variable name.");

    while ($c = fgetc(STDIN)) {
        if (ctype_alnum($c) || preg_match("/[_|-|&|$|%|*]/", $c) == 1) {
            $str.=$c;
            continue;
        }
        if (strcmp($c, "\t") == 0 || strcmp($c, " ") == 0)
            break;
        if (strcmp($c, "\n") == 0 && $end == true)
            break;
        else
            terminate(21, "Wrong variable name.");            
    }
    return $str;
}


function getTyp($end) {
    $tmp;
    $type = getFrame($frame, true); 
    if (getFrame($frame, true) != 1) {
        // read to end of line
        if ($end == true) {
            if (ctype_space(fgets(STDIN)) != 1)
                terminate(21, "Invalid arguments of instruction.");
        }
        else {
            $c = fgetc(STDIN);
            if (strcmp($c, " ") == 0 || strcmp($c, "\t") == 0) {
                switch (variable) {
                    case 'value':
                        # code...
                        break;
                    
                    default:
                        # code...
                        break;
                }
            }
        }
    }
}

function getInst() {
    $op_code = skipWhite();

    while ($op_code == "\n")
        $op_code = skipWhite();

    if ($op_code == EOF)
        return EOF;

    //global $help_arr, $inst_0, $inst_1, $inst_2, $inst_3;
    global $all_inst;

    $no_arg = false;
    //$inst = new Instruction();
    $c;
    while ($c = fgetc(STDIN)) {
        if ($c == " " || $c == "\t")
            break;
        if (ctype_alnum($c)) {
            $op_code.=$c;
        }
        else if ($c == "\n") {
            $no_arg = true;
        }
        else {
            return ERR;
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
                    return ERR;
            }
            return $op_code;
        }
    }
    // Unknown operation code
    return ERR;
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
        if ($var == EOF) {
            break;
        }
        // Unknown instruction
        if ($var == ERR) {
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
                    if ($str == ERR)
                        return 21;
                    $xml_arg->addAttribute("type", "var");
                    $xml_arg[0] = $str;
                    break;
                case 2:
                    $str = getVarLab($end, false);
                    if ($str == ERR)
                        return 21;
                    $xml_arg->addAttribute("type", "label");
                    $xml_arg[0] = $str;
                    break;
                case 3:
                    //$c = skipWhite();
                    //if ()
                    $type;
                    $str = getSymb($end, $type);
                    if ($str == ERR)
                        return 21;
                    switch ($type) {
                        case 1:
                            $xml_arg->addAttribute("type", "var");
                            $xml_arg[0] = $str;
                            break;
                        case 2:
                            $xml_arg->addAttribute("type", "int");
                            $xml_arg[0] = strstr($str, "@");
                            break;
                        case 3:
                            $xml_arg->addAttribute("type", "string");
                            $xml_arg[0] = strstr($str, "@");
                            break;
                        case 4:
                            $xml_arg->addAttribute("type", "bool");
                            $xml_arg[0] = strstr($str, "@");
                            break;
                        default:
                            // WTF
                            break;
                    }
                    break;
                case 4:
                    $str = getTyp($end);
                    if ($str == ERR)
                        return 21;
                    $xml_arg->addAttribute("type", "type");
                    $xml_arg[0] = $str;
                    break;
                default:
                    // WTF
                    break;
            }
           // echo $xml_el->asXml();
        }
    }
    
    echo $xml_el->asXml();
    return 0;
}

return main();

?>
