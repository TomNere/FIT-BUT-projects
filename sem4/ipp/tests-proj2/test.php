<?php
/**
*  Course:      Principles of Programming Languages (IPP)
*  Project:     Automated script for testing functionality of both parse.php and interpret.py 
*  File:        test.php
*
*  Author: Tomáš Nereča : xnerec00
*/

/************************CONSTANTS**************************/
const ARG_ERR = 10;
const SYN_ERR = 21;
const OK_VAL = 0;
const ARG_MESS = "Nesprávná kombinace argument, try --help.";

/************************GLOBAL VARIABLES************************/

$parse_file = "./parse.php";
$int_file = "./interpret.php";
$directory = "./";
$recursive = false;
$dir_array = Array ();


/** Write error message and exit
* $code - exit code
* $str - error message
*/
function terminate($code, $str) {
    fwrite(STDERR, $str."\n");
    exit($code);
}

/** Check for valid arguments, show help or set variables for extension
*/
function argHandle() {
    global $argc, $parse_file, $interpret_file, $directory, $recursive;
    if ($argc == 1)                                         // No arguments
        terminate($ARG_ERR, ARG_MESS);

    $options = array("help", "recursive", "directory:", "parse-script:", "int-script:");
    $params = getopt("", $options);

    if ($params == false)
        terminate(ARG_ERR, ARG_MESS);

    if ($argc == 2 && isset($params["help"])) {    
        print ( "Použití:\n".
                "   php5.6 test.php --help\n".
                "   php5.6 test.php [--parse-script=file] [--int-script=file] [--directory=path]\n".
                "                   [--recursive]\n\n".
                "   --help                  Zobrazí nápovědu\n\n".
                "   --parse-script=file     Soubor se skriptem parse.php\n".
                "                           (chybí-li, implicitní hodnota je aktuální adresář)\n\n".
                "   --int-script=file       Soubor se skriptem interpret.py\n".
                "                           (chybí-li, implicitní hodnota je aktuální adresář)\n\n".
                "   --directory=path        Adřesář s testovacími soubory\n".
                "                           (chybí-li, tak skript prochází aktuální adresář)\n\n".
                "   --recursive             Skript hledá testovací soubory rekurzivně\n".
                "                           ve všech podadresářích\n");
        exit(0);
    }
    else if ($argc != 2 && isset($params["help"])) {
        terminate(ARG_ERR, ARG_MESS);
    }
    else {
        if (isset($params["parse-script"])) {
            $parse_file = $params["parse-script"];
        }
        if (isset($params["int-script"])) {
            $int_file = $params["int-script"];
        }
        if (isset($params["directory"])) {
            $directory = $params["directory"];
        }
        if (isset($params["recursive"])) {
            $recursive = true;
        }
    }
}
/* Get all directories - for --recursive argument
*/
function getDirs() {
    global $directory, $dir_array;
    $rec_iter = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($directory)); 

    foreach ($rec_iter as $file) {
        if ($file->isDir())
        array_push($dir_array, $file->getPathname());
    }
}


/*************************************MAIN FUNCTION**************************************/

argHandle();                            // Parse arguments
array_push($dir_array, $directory);     // Add first directory to array of all directories

if ($recursive) {                        // Get array of all subdirectories
    getDirs();
}

var_dump($dir_array);

?>
