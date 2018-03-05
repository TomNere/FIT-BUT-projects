<?php
/**
*  Course:      Principles of Programming Languages (IPP)
*  Project:     Implementation of the IPPcode18 imperative language interpreter
*  File:        parse.php
*
*  Author: Tomáš Nereča : xnerec00
*/

/************************CONSTANTS**************************/
const ARG_ERR = 10;
const SYN_ERR = 21;
const EOF = -1;
const ERR = -2;

/**********************GLOBAL VARIABLES*********************/
$params;
$file;
$stats = false;
$line_count = 1;
$comment_count = $inst_count = 0;

/** Array of all instructions 
* KEY - operation code
* VALUE - array of parameters:
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

/** Write error message and exit
* $code - exit code
* $str - error message
*/
function terminate($code, $str) {
    global $line_count;
    if ($code == SYN_ERR)
        fwrite(STDERR, "Error found in line ".$line_count."!\n");
    fwrite(STDERR, $str."\n");
    exit($code);
}

/** Skip all white characters
* return value - first non-white character or EOF
*/
function skipWhite() {
    $c;
    while ($c = fgetc(STDIN)) {                 // Repeat until EOF
        if (strcmp($c, chr(35)) == 0) {         // # - comment
            skipComment();
            return "#";
        }
        else if (strcmp($c, "\n") == 0) {       // New line
            $GLOBALS['line_count']++;
            return "\n";
        }
        if (!ctype_space($c))
            return $c;
    }
    return EOF;
}

/** Skip character to the end of line and increment counters
*/
function skipComment() {
    $GLOBALS['comment_count']++;
    $GLOBALS['line_count']++;
    fgets(STDIN);
}

/** Check for new line or comment
* $c - character to check
* return value - true if new line or comment or space/tab detected
*              - false if not
*/
function nlcCheck($c) {
    if (strcmp($c, "\n") == 0) {
        $GLOBALS['line_count']++;
        return true;
    }
    if (strcmp($c, chr(35)) == 0) {
        skipComment();
        return true;
    }
    if (strcmp($c, "\t") == 0 || strcmp($c, " ") == 0)
        return true;
    return false;
}

/** Check proper escape sequence after \
* return value - escape sequence number 
*/
function getEscape() {
    $number = fgets(STDIN, 4);                  // Get 3 characters
    if (!ctype_digit($number))                  // 000-999
        terminate(SYN_ERR, "Escape sequence expected.");
    return $number;
}

/** Read string constant
* return value - string constant
*/
function getString() {
    global $line_count, $comment_count;
    $c;
    $str = "";

    while($c = fgetc(STDIN)) {
        if (nlcCheck($c))                                       // End of string
            break;
        if (strcmp($c, chr(92)) == 0) {                         // Escape sequence
            $str = $str.chr(92).getEscape();
            continue;
        }
        if (!ctype_space($c))
            $str.=$c;
        else
            terminate(SYN_ERR, "Invalid string.");              // Some invalid character
    }
    return (htmlspecialchars($str, ENT_XML1, 'UTF-8'));          // Translate XML special characters
}

/** Read integer constant
* return value - integer constant
*/
function getInt() {
    $c = fgetc(STDIN);
    $number;

    if (is_numeric($c) || strcmp($c, "+") == 0 || strcmp($c, "-") == 0)     // Optional + or -
        $number = $c;
    else
        terminate(SYN_ERR, "Invalid integer constant.");

    while ($c = fgetc(STDIN)) {             // Check for digits
        if (is_numeric($c))
            $number.=$c;
        else
            break;
    }

    if (nlcCheck($c))                               // Valid number
        return $number;

    terminate(SYN_ERR, "Invalid integer.");                 
}

/** Read symbol - constant or variable
* $type - pointer to type of symbol - var, int, bool or string
* return value - constant or variable name
*/
function getSymb(&$type) {
    $frame;
    $type = getFrame($frame, false);                // Check for frame or constant type

    switch ($type) {
        case 1:                                     // Variable name
            $type = "var";
            return $frame.getVarLab(false);
            break;
        case 2:                                     // Integer constant
            $type = "int";
            return getInt();
            break;
        case 3:                                     // String constant
            $type = "string";
            return getString();
            break;
        case 4:                                     // Bool constant
            $type = "bool";
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
            else
                terminate(SYN_ERR, "Invalid bool value.");

            if (!nlcCheck($c))                       // End of argument
                terminate(SYN_ERR, "Invalid bool value.");                
            break;
        default:
            terminate(SYN_ERR, "Unknown error.");
            break;
    }
}

/** Get frame(GF, LF, TF) or type(int, string, bool)
* $frame - pointer to string where the name of frame is stored
* $type - if true, '@' isn't expected after type name
* return value - 1(frame), 2(int), 3(string), 4(bool)
*/
function getFrame(&$frame, $is_type) {
    $frame = skipWhite();
    $frame = $frame.fgets(STDIN, 3);               // Get next 2 characters

    if (preg_match("/[G|L|T|]F@/", $frame))        // GF@ LF@ or TF@
        return 1;
    
    if (ctype_lower($frame)) {                     // Possible type name
        $c;
        while ($c = fgetc(STDIN)) {
            if (ctype_lower($c))
                $frame.=$c;
            else
                break;
        }

        if (strcmp($c, "@") != 0 && $is_type == false)  // If is_type is false, @ is expected after type
            terminate(SYN_ERR, "@ expected.");

        if ($is_type == true) {                             
            if (!nlcCheck($c))                          // End of argument
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

/** Check for valid variable or label name
* is_var - if true, frame in front of variable name is expected
* return value - string - name of variable/label
*/
function getVarLab($is_var) {
    $str = "";

    if ($is_var == true) {                          // Check for frame
        $is_frame = getFrame($str, false);
        if ($is_frame != 1)
            terminate(SYN_ERR, "Variable name expected.");
    }
 
    $c = fgetc(STDIN);
    if (ctype_alpha($c) || preg_match("/[_|-|&|$|%|*]/", $c) == 1)  // Name has to start with alpha or special character
        $str.=$c;
    else
        terminate(SYN_ERR, "Wrong variable name.");

    while ($c = fgetc(STDIN)) {
        if (ctype_alnum($c) || preg_match("/[_|-|&|$|%|*]/", $c) == 1) {    // Next characters could be alfanumeric or special
            $str.=$c;
            continue;
        }
        if (nlcCheck($c))                          // End of argument
            break;
        else
            terminate(SYN_ERR, "Wrong variable name.");            
    }
    return (htmlspecialchars($str, ENT_XML1, 'UTF-8'));          // Translate XML special character &
}

/** Check for valid operation code
* return value - string containing opcode
*/
function getInst() {
    $op_code = skipWhite();

    while (strcmp($op_code, "\n") == 0 || strcmp($op_code, "#") == 0)   // Skip empty lines and comments
        $op_code = skipWhite();

    if ($op_code == EOF)
        return EOF;

    global $all_inst;

    $c;
    $no_arg = false;                                            // Signalize operation code without arguments

    while ($c = fgetc(STDIN)) {
        if (strcmp($c, " ") == 0 || strcmp($c, "\t") == 0) {
            break;
        }
        if (nlcCheck($c)) {                                     // Operation code must not have arguments
            $no_arg = true;
            break;
        }
        if (ctype_alnum($c))
            $op_code.=$c;
        else
            terminate(SYN_ERR, "Unknown operation code.");
    }
    $op_code = strtolower($op_code);                            // Case insentive

    foreach ($all_inst as $key => $value) {
        if (strcmp($key, $op_code) == 0) {
            if ($no_arg == true) {                              // Ckeck if operation code have no arguments
                if (count($all_inst[$key]) != 0)
                    terminate(SYN_ERR, "Argument(s) after operation code expected.");
            }
            return $op_code;
        }
    }
    terminate(SYN_ERR, "Unknown operation code.");
}

/** Write information to file
*/
function extension() {
    global $stats, $file, $params, $comment_count, $inst_count;

    if ($stats) {
        $handle = fopen($file, "w");
        if ($handle == false)
            terminate(12, "Unable to open file.");
        foreach ($params as $key => $value) {
            if (strcmp($key, "loc") == 0)
                fwrite($handle, $inst_count."\n");
            if (strcmp($key, "comments") == 0)
                fwrite($handle, $comment_count."\n");
        }
    }
}

/** Check for valid arguments, show help or set variables for extension
*/
function argHandle() {
    global $argc, $params, $stats, $file;
    if ($argc == 1)                                         // No arguments
        return;

    $options = array("help", "stats:", "comments", "loc");
    $params = getopt("", $options);

    if ($params == false)
        terminate(ARG_ERR, "Wrong arguments, try --help.");

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
        else if (isset($params["stats"])) {
            $stats = true;
            $file = $params["stats"];
        }
        else
            terminate(ARG_ERR, "Wrong arguments, try --help.");
    }
    else if ($argc == 3) {
        if (isset($params["stats"]) && (isset($params["loc"]) || isset($params["comments"]))) {
            $stats = true;
            $file = $params["stats"];
        }
        else
            terminate(ARG_ERR, "Wrong arguments, try --help.");
    }
    else if ($argc == 4) {
        if (!isset($params["stats"]) || !isset($params["loc"]) || !isset($params["comments"]))
            terminate(ARG_ERR, "Wrong arguments, try --help.");
        $stats = true;
        $file = $params["stats"];
    }
    else
        terminate(ARG_ERR, "Wrong arguments, try --help.");
}

/*******************************Main function**************************/

argHandle();                                            // Check for arguments

// First line check
$tmp = rtrim(fgets(STDIN));

    if (strcasecmp($tmp, ".ippcode18") != 0)            // Case insensitive 
        terminate(SYN_ERR, ".IPPcode18 is missing!");
 
// Create basic XML
$xml_el = new SimpleXMLElement('<?xml version="1.0" encoding="UTF-8"?><program language="IPPcode18"></program>');

$actual_line;                       // Number of actual line - signalize if \n was detected

while (true) {                      // Repeat until error or EOF
    $op_code = getInst();

    if ($op_code == EOF)            // End of file
        break;

    $inst_count++;

    $xml_inst = $xml_el->addChild("instruction");
    $xml_inst->addAttribute("order", $inst_count);
    $xml_inst->addAttribute("opcode", strtoupper($op_code));

    $actual_line = $line_count;     // Set actual line

    foreach ($all_inst[$op_code] as $key => $value) {       // Get arguments
        $xml_arg = $xml_inst->addChild("arg".strval($key + 1));

        switch ($value) {                                   // type of argument
            case 1:                                         // Variable
                $xml_arg->addAttribute("type", "var");
                $xml_arg[0] = getVarLab(true);              // true - frame is expected
                break;
            case 2:                                         // Label
                $xml_arg->addAttribute("type", "label");
                $xml_arg[0] = getVarLab(false);             // false - frame isn't expected
                break;
            case 3:                                         // Symbol(variable or constant)
                $type;
                $str = getSymb($type);
                $xml_arg->addAttribute("type", $str);
                $xml_arg[0] = $str;
                break;
            case 4:                                         // Type
                $xml_arg->addAttribute("type", "type");
                $xml_arg[0] = getFrame($frame, true);
                break;
            default:
                terminate(SYN_ERR, "Unknown error.");
                break;
        }
        if ((count($all_inst[$op_code]) > ($key + 1)) && (($line_count - $actual_line) != 0))
            terminate(SYN_ERR, "Missing argument(s).");
    }

    if (($line_count - $actual_line) == 0) {                    // Check for new line after arguments
        $c = skipWhite();
        if (strcmp($c, "\n") && strcmp($c, "#") && $c != EOF)   // Unexpected character after arguments
            terminate(SYN_ERR, "Unexpected argument(s).");    
    }
    else if (($line_count - $actual_line) != 1)
        terminate(SYN_ERR, "Instruction on multiple lines.");
}

extension();
print $xml_el->asXml();

?>
