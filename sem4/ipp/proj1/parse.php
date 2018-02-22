<?php
/*******************CONSTANTS DEFINITIONS********************/
const ARG_ERR = 10;
const SYN_ERR = 21;
const EOF = -1;
const ERR = -2;

/******************GLOBAL VARIABLES**************************/
$params;
$file;
$stats = $comments = $loc = false;
$line_count = 1;
$comment_count = 0;

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
    fwrite(STDERR, "Error found in line ".$line_count."!\n".$str."\n");
    exit($code);
}

// Skip white characters
function skipWhite() {
    $c;
    while (true) {
        if (feof(STDIN))
            return EOF;

        $c = fgetc(STDIN);

        if (strcmp($c, chr(35)) == 0) {
            $comment_count++;
            $line_count++;
            fgets(STDIN);
            continue;
        }
        if (strcmp($c, "\n") == 0) {
            $line_count++;
            return "\n";
        }
        if (ctype_space($c))
            continue;
        else
            return $c;
    }
}

// Read Escape sequence
function getEscape() {
    $number = "";
    $number = fgets(STDIN, 4);
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
        terminate(SYN_ERR, "Escape sequence expected.");
}

// Read string
function getString($end) {
    $c;
    $str = "";

    while($c = fgetc(STDIN)) {
        if (((strcmp($c, "\n") == 0) && $end)) {
            $line_count++;
            break;
        }
        if ((strcmp($c, " ") == 0) || (strcmp($c, "\t") == 0))
            break;
        if (strcmp($c, chr(92)) == 0) {
            $tmp = getEscape();
            $str.=chr(92);
            $str.=$tmp;
            continue;
        }
        // Comment
        if (strcmp($c, chr(35)) == 0 && $end) {
            fgets(STDIN);
            $comment_count++;
            $line_count++;
            break;
        }
        if (ctype_print($c))
            $str.=$c;
        else
            terminate(SYN_ERR, "Invalid string.");
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
        terminate(SYN_ERR, "Invalid int.");

    while ($c = fgetc(STDIN)) {
        if (is_numeric($c))
            $number.=$c;
        else
            break;
    }

    if ((strcmp($c, "\n") == 0 && $end)) {
        $line_count++;
        return $number;
    }
    else if (strcmp($c, chr(35)) == 0 && $end) {
        fgets(STDIN);
        $comment_count++;
        $line_count++;
        return $number;
    }
    if (strcmp($c, "\t") == 0 || strcmp($c, " ") == 0)
        return $number;

    terminate(SYN_ERR, "Invalid int.");
}

// Constant or variable
function getSymb($end, &$type) {
    $frame;
    $type = getFrame($frame, false, $end);

    switch ($type) {
        case 1:
            return $frame."@".getVarLab($end, false);
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
                else if ((strcmp($c, "\n") == 0 && $end)) {
                    $line_count++;
                    break;
                }
                else if (strcmp($c, chr(35)) == 0 && $end) {
                    fgets(STDIN);
                    $comment_count++;
                    $line_count++;
                }
                else if (strcmp($c, "\t") == 0 || strcmp($c, " ") == 0)
                    break;
                else
                    terminate(SYN_ERR, "Invalid bool value.");
            }
            if (strcmp($tmp, "true") == 0 || strcmp($tmp, "false") == 0)
                return $tmp;
            else
                terminate(SYN_ERR, "Invalid bool value.");
            break;
        default:
            terminate(SYN_ERR, "Unknown error.");
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
function getFrame(&$frame, $is_type, $end) {
    $frame = skipWhite();

    if (strcmp($frame, "L") == 0 || strcmp($frame, "T") == 0 || strcmp($frame, "G") == 0) {

        if (strcmp(fgetc(STDIN), "F") == 0)
            $frame.="F";
        else
            terminate(SYN_ERR, "Invalid frame name.");

        if (strcmp(fgetc(STDIN), "@") != 0)
            terminate(SYN_ERR, "@ expected.");
        else {
            $frame.="@";
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
            terminate(SYN_ERR, "@ expected.");

        if ($is_type == true) {
            if (strcmp($c, "\n") == 0 && $end)
                $line_count++;
            else if ((strcmp($c, "\t") == 0 || strcmp($c, " ") == 0))
                //
            else if (strcmp($c, chr(35)) == 0 && $end) {
                fgets(STDIN);
                $comment_count++;
                $line_count++;
            }
            else
                terminate(SYN_ERR, "Unknown type.");
        }   

        if (strcmp($frame, "int") == 0)
            return 2;
        if (strcmp($frame, "string") == 0)
            return 3;
        if (strcmp($frame, "bool") == 0)
            return 4;
    }
    terminate(SYN_ERR, "Unknown type of symbol.");
}

// Variable or label
function getVarLab($end, $is_var) {
    $str = "";

    if ($is_var == true) {
        $is_frame = getFrame($str, false, $end);

        if ($is_frame != 1)
            terminate(SYN_ERR, "Variable name expected.");
        $str.="@";
    }
 
    // Must start with alpha or special
    $c = fgetc(STDIN);
    if (ctype_alpha($c) || preg_match("/[_|-|&|$|%|*]/", $c) == 1)
        $str.=$c;
    else
        terminate(SYN_ERR, "Wrong variable name.");

    while ($c = fgetc(STDIN)) {
        if (ctype_alnum($c) || preg_match("/[_|-|&|$|%|*]/", $c) == 1) {
            $str.=$c;
            continue;
        }
        if (strcmp($c, "\t") == 0 || strcmp($c, " ") == 0)
            break;
        if (strcmp($c, "\n") == 0 && $end == true) {
            $line_count++;
            break;
        }
        if (strcmp($c, chr(35)) == 0 && $end) {
                fgets(STDIN);
                $comment_count++;
                $line_count++;
                break;
        }
        else
            terminate(SYN_ERR, "Wrong variable name.");            
    }
    return $str;
}


function getTyp($end) {
    $frame;
    $type = getFrame($frame, true, $end); 
    if ($type != 1)
        return $frame;

    terminate(SYN_ERR, "Type expected.");
}

function getInst() {
    $op_code = skipWhite();

    while (strcmp($op_code, "\n") == 0)
        $op_code = skipWhite();

    if ($op_code == EOF)
        return EOF;

    global $all_inst;

    $c;
    $no_arg = false;

    while ($c = fgetc(STDIN)) {
        if (strcmp($c, " ") == 0 || strcmp($c, "\t") == 0) {
            break;
        }
        if (strcmp($c, "\n") == 0) {
            $line_count++
            $no_arg = true;
        }
        if (strcmp($c, chr(35)) == 0) {
            fgets(STDIN);
            $comment_count++;
            $line_count++;
            $no_arg = true;
            break;
        }
        if (ctype_alnum($c))
            $op_code.=$c;
        else
            terminate(SYN_ERR, "Unknown instruction");
    }
    // Case insentive
    $op_code = strtolower($op_code);

    foreach ($all_inst as $key => $value) {
        if (strcmp($key, $op_code) == 0) {
            // \n or # after operation code
            if ($no_arg == true) {
                if (count($all_inst[$key]) != 0)
                    terminate(SYN_ERR, "Argument(s) expected.");
            }
            return $op_code;
        }
    }
    // Unknown operation code
    terminate(SYN_ERR, "Unknown instruction");
}

function argHandle() {
    global $argc, $params, $stats, $file, $comments, $loc;
    // No arguments
    if ($argc == 1)
        return;

    $options = array("help", "file:", "comments", "loc");
    $params = getopt("", $options);

    if ($params == false) {
        terminate(ARG_ERR, "Wrong arguments, try --help.");
    }

    if ($argc == 2) {
        if (isset($params["help"])) {
            print ( "Usage:\n".
                    "   php5.6 parse.php\n".
                    "   php5.6 parse.php --help\n".
                    "   php5.6 parse.php --stats=file [--loc] [--comments]\n\n".
                    "   --help       This help\n".
                    "   --stats=file Statistics about source file are logged to given file\n".
                    "   --loc        Number of lines with instructions are logged\n".
                    "   --comments   Number of lines with comments are logged\n");
            exit(0);
        }
        else if (isset($params["file"])) {
            $stats = true;
            $file = $params["file"];
        }
        else
            terminate(ARG_ERR, "Wrong arguments, try --help.");
    }
    else if ($argc == 3) {
        if (isset($params["help"]))
            terminate(ARG_ERR, "Wrong arguments, try --help.");
        if (isset($params["file"])) {
            $stats = true;
            $file = $params["file"];
        }
        if (isset($params["comments"]))
            $comments = true;
        else
            $loc = true;
    }
    else if ($argc == 4) {
        if (isset($params["help"]))
            terminate(ARG_ERR, "Wrong arguments, try --help.");
        $stats = true;
        $file = $params["file"];
        $loc = true;
        $comments = true;
    }
    else
        terminate(ARG_ERR, "Wrong arguments, try --help.");
}

function main() {
    // Arguments check
    argHandle();

    // First line check
    $tmp = skipWhite();
    $tmp = $tmp.rtrim(strtolower(fgets(STDIN)));

    if (strcmp($tmp, ".ippcode18") != 0)
        terminate(SYN_ERR, ".IPPcode18 is missing!");

    // Basic XML string
$xml_str = <<<XML
<?xml version="1.0" encoding="UTF-8"?>
<program language="IPPcode18">
</program>
XML;
    
    $xml_el = new SimpleXMLElement($xml_str);

    global $inst_count, $all_inst, $line_count;

    // $end signalize last parameter
    $end;

    $actual_line;

    // Repeat until error or EOF
    while (true) {
        $op_code = getInst();
        // End of file
        if ($op_code == EOF)
            break;

        $inst_count++;

        $xml_inst = $xml_el->addChild("instruction");
        $xml_inst->addAttribute("order", $inst_count);
        $xml_inst->addAttribute("opcode", strtoupper($op_code));

        // For instrucions without arguments
        if (count($all_inst[$op_code]) == 0)
            continue;

        $actual_line = $line_count;

        foreach ($all_inst[$op_code] as $key => $value) {
            if (count($all_inst[$op_code]) == ($key + 1)) 
                $end = true;
            else
                $end = false;

            $xml_arg = $xml_inst->addChild("arg".strval($key + 1));

            switch ($value) {
                case 1:
                    $xml_arg->addAttribute("type", "var");
                    $xml_arg[0] = getVarLab($end, true);
                    break;
                case 2:
                    $xml_arg->addAttribute("type", "label");
                    $xml_arg[0] = getVarLab($end, false);
                    break;
                case 3:
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
                    $xml_arg[0] = $str;
                    break;
                case 4:
                    $xml_arg->addAttribute("type", "type");
                    $xml_arg[0] = getTyp($end);
                    break;
                default:
                    // WTF
                    break;
            }
        }
        // Check for valid format
        if (($line_count - $actual_line) == 0) {
            $c;
            while ($c = fgetc(STDIN)) {
                if (strcmp($c, " ") == 0 || strcmp($c, "\t") == 0)
                    continue;
                if (strcmp($c, chr(35)) == 0) {
                    fgets(STDIN);
                    $comment_count++;
                    $line_count++;
                    break;
                }
                if (strcmp($c, "\n") == 0) {
                    $line_count++;
                    break;
                }
                else
                    terminate(SYN_ERR, "End of line expected.");
            }
        }
        else if (($line_count - $actual_line) != 1)
            terminate(SYN_ERR, "Instruction on multiple lines.");
    }
    
    print $xml_el->asXml();
}

main();

?>
