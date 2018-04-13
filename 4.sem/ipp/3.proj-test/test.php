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
const IN_ERR = 11;
const SYN_ERR = 21;
const OK_VAL = 0;
const PHP_RUN = 'php56 ';
const PYTHON_RUN = 'python3.6 ';
const ARG_MESS = "Nesprávná kombinace parametru. Zkuste parametr --help pro zobrazeni napovedy.";

/************************GLOBAL VARIABLES************************/

$parse_file = "./parse.php";
$int_file = "./interpret.py";
$directory = ".";
$recursive = false;
$dir_array = Array();


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
    global $argc, $parse_file, $int_file, $directory, $recursive;

    $options = array("help", "recursive", "directory:", "parse-script:", "int-script:");
    $params = getopt("", $options);

    if ($params == false)
        terminate(ARG_ERR, ARG_MESS);

    if ($argc == 2 && isset($params["help"])) {    	// Show help
        print ( "Použití:\n".
                "   php5.6 test.php --help\n".
                "   php5.6 test.php [--parse-script=file] [--int-script=file] [--directory=path]\n".
                "                   [--recursive]\n\n".
                "   --help                  Zobrazí nápovědu\n\n".
                "   --parse-script=file     Soubor se skriptem parse.php\n".
                "                           (chybí-li, implicitní hodnota je skript v aktuálním adresáři)\n\n".
                "   --int-script=file       Soubor se skriptem interpret.py\n".
                "                           (chybí-li, implicitní hodnota je skript v aktuálním adresáři)\n\n".
                "   --directory=path        Adřesář s testovacími soubory\n".
                "                           (chybí-li, tak skript prochází aktuální adresář)\n\n".
                "   --recursive             Skript hledá testovací soubory rekurzivně\n".
                "                           ve všech podadresářích\n");
        exit(0);
    }
    else if ($argc != 2 && isset($params["help"])) {	// Invalid parameters
        terminate(ARG_ERR, ARG_MESS);
    }
    else {												// Set values in accordance with parameters
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

/* Get all directories - for --recursive parameter
*/
function getDirs() {
    global $directory, $dir_array;

    // Check if directory exists
	if (!file_exists($directory)) {
		terminate(IN_ERR, "Directory not found.");
	}

    $rec_iter = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($directory)); 

    foreach ($rec_iter as $dir) {
        // Check for duplicates
        if ($dir->isDir() && !in_array(realpath($dir), $dir_array) )
            array_push($dir_array, realpath($dir->getPathname()).'/');
    }
}

// Add new line at the end of string in arr
function addNewLine(&$arr) {
	foreach ($arr as $key => $value) {
		$arr[$key]=$value.PHP_EOL;
	}
}

/*************************************MAIN FUNCTION**************************************/

argHandle();                            // Parse arguments

if ($recursive) {                       // Get array of all subdirectories
    getDirs();
    //var_dump($dir_array);
}
else {
	if (!file_exists($directory)) {
		terminate(IN_ERR, "Directory not found.");
	}
    array_push($dir_array, realpath($directory).'/'); // Only 1 directory
    //var_dump($dir_array);
}

// Exist check
if (!file_exists($parse_file)) {
	terminate(IN_ERR, "parse.php not found.");
}
if (!file_exists($int_file)) {
	terminate(IN_ERR, "interpret.py not found.");
}

// Absolute path is better
$parse_file = realpath($parse_file);

$int_file = realpath($int_file);

// HTML
print(
"<!DOCTYPE html>\n
    <html lang=\"cs\">\n
    <head>\n
        <meta charset=\"utf-8\">\n
        <title>IPP-prehled testu</title>\n
    </head>\n
    <body style=\"height:100%;font-family: 'arial'; margin: 0px;padding: 0px;background-color: LightCyan;\">\n
        <div style=\"text-align:center;vertical-align: middle;line-height: 61px;height: 61px;background-color: rgba(20, 61, 102, 1);color: white;\";><h1 style=\"margin-top: 0;\">Přehled úspěšnosti testů pro parse.php a interpret.py</h1></div>\n"
);

$test_count = 0;
$success_count = 0;

// Create temporary file for XML and interpret output
$tmp_f = tmpfile();
$tmp_f_uri = stream_get_meta_data($tmp_f);

foreach ($dir_array as $key_d => $value_d) {
    $file_array = scandir($value_d);
    // Skip directories without test files
    $something = preg_grep ('/^.*\.(src)$/i', $file_array);
    if (empty($something))	// If no .src file, continue
        continue;

    print("<div style=\"background-color:PowderBlue;\">\n<h1 style=\"margin-top: 0;color:navy;\">Testy z adresáře:&emsp;&emsp;".htmlentities($value_d, ENT_HTML5, "utf-8")."</h1>\n<table style=\"font-size: 18pt;\">");

    $dir_success = 0;
    $dir_count = 0;
    foreach ($file_array as $key_f => $value_f) {
        // Find .src file
        if (!strcmp(substr($value_f, -4), '.src')) {
            $test_count++;
            $dir_count++;

            $file_name = substr($value_f, 0, -4);

            if (!in_array($file_name.'.rc', $file_array)) {
                // Create rc file with 0
                file_put_contents($value_d.$file_name.'.rc', "0\n");
            }
            if (!in_array($file_name.'.in', $file_array)) {
                // Create rc file with 0
                file_put_contents($value_d.$file_name.'.in', "");
            }
            if (!in_array($file_name.'.out', $file_array)) {
                // Create rc file with 0
                file_put_contents($value_d.$file_name.'.out', "");
            }

            $returned_code;
            $output;

            print("<tr><th style=\"text-align: left;\">Testovací soubor: ".htmlentities($file_name, ENT_HTML5, "utf-8")."&emsp;&emsp;</th>");

            // Exec command
            $command = PHP_RUN.escapeshellarg($parse_file).' < '.escapeshellarg($value_d.$file_name).'.src';
            //fwrite(STDERR, $command);
            exec($command, $output, $returned_code);

            // If error in parse.php occured
            if ($returned_code != 0) {
                $rc_file = fopen($value_d.$file_name.'.rc', 'r');
                $rc = fread($rc_file, 2);	// Max RC is 99
                fclose($rc_file);

                if (!strcmp($rc, $returned_code)) {
                    $dir_success++;
                    $success_count++;
                    print("<th style=\"text-align: left;\">parse.php rc OK</th><th style=\"text-align: left; font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: green;font-weight: bold;\">&emsp;&#10004;&emsp;Úspěch</th></tr>");
                }
                else {
                    print("<th style=\"text-align: left;\">parse.php očekávaný rc - ".$rc.", aktuální rc - ".$returned_code."</th><th style=\"text-align: left; font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: red;font-weight: bold;\">&emsp;&#10007;&emsp;Neúspěch</th></tr>");
                }
            }
            else {
                // Print parse.php output to this file
                file_put_contents($tmp_f_uri['uri'], $output);

                // Clean output of parser and exec command
                unset($output);
                $command = PYTHON_RUN.$int_file." --source=".$tmp_f_uri['uri'].' < '.escapeshellarg($value_d.$file_name).'.in';
                exec($command, $output, $returned_code);

                // If error in interpret.py occured
                if ($returned_code != 0) {
                    $rc_file = fopen($value_d.$file_name.'.rc', 'r');
                    $rc = fread($rc_file, 2);	// Max RC is 99
                    fclose($rc_file);

                    if (!strcmp($rc, $returned_code)) {
                        $dir_success++;
                        $success_count++;
                        print("<th style=\"text-align: left;\">interpret.py rc OK</th><th style=\"text-align: left; font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: green;font-weight: bold;\">&emsp;&#10004;&emsp;Úspěch</th></tr>");
                    }
                    else {
                        print("<th style=\"text-align: left;\">interpret.py očekávaný rc - ".$rc.", aktuální rc - ".$returned_code."</th><th style=\"text-align: left; font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: red;font-weight: bold;\">&emsp;&#10007;&emsp;Neúspěch</th></tr>");
                    }
                }
                // Check for diff
                else {
                	addNewLine($output);								// Add new lines to output be correct
                    file_put_contents($tmp_f_uri['uri'], $output);		// File created at parser test
                    unset($output);
                    $command = "diff ".escapeshellarg($value_d.$file_name).'.out '.$tmp_f_uri['uri'];
                    exec($command, $output, $returned_code);

                    if (!isset($output[0])) {		// Diff in files
                        $dir_success++;
                        $success_count++;
                        print("<th style=\"text-align: left;\">analýza a interpretace OK</th><th style=\"text-align: left; font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: green;font-weight: bold;\">&emsp;&#10004;&emsp;Úspěch</th></tr>");
                    }
                    else {
                        print("<th style=\"text-align: left;\">Rozdíl ve výstupu interpretu</th><th style=\"text-align: left; font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: red;font-weight: bold;\">&emsp;&#10007;&emsp;Neúspěch</th></tr>");
                    }
                }
            }
        }
    }

    print("</table>\n<hr>
        </div>\n
        <div style=\"height: 40px;font-size: 22pt;font-weight: bold;\">&emsp;&emsp;Úspěšnost testů z daného adresáře: ".$dir_success."/".$dir_count.".</div><hr style=\"padding: 0px; margin: -1px; \">"); 
}
print("<div style=\"border:5px solid black;height:81px;line-height:81px;vertical-align: middle;background-color: PaleGreen;text-align: center;font-size: 30pt;font-weight: bold;\"> Celková úspěšnost všech testů: ".$success_count."/".$test_count.".</div>
        </body>\n</html>");
fclose($tmp_f);
?>
