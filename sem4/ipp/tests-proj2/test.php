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
$int_file = "./interpret.py";
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
        // Check for duplicates
        if ($file->isDir() && !in_array(substr($file, 0, strlen($file) - 1), $dir_array) )
            array_push($dir_array, $file->getPathname());
    }
}


/*************************************MAIN FUNCTION**************************************/

argHandle();                            // Parse arguments

if ($recursive) {                       // Get array of all subdirectories
    getDirs();
}
else {
    array_push($dir_array, $directory.'/.'); // Only 1 directory
}

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

foreach ($dir_array as $key_d => $value_d) {
    $file_array = scandir($value_d);

    // Skip directories without test files
    $something = preg_grep ('/^.*\.(src)$/i', $file_array);
    if (empty($something))
        continue;

    print("<div style=\"background-color:PowderBlue;\">\n<h1 style=\"margin-top: 0;color:navy;\">Testy z adresáře:&emsp;&emsp;".$value_d."</h2>\n<ol style=\"font-size: 18pt;\">\n");

    $dir_success = 0;
    $dir_count = 0;
    foreach ($file_array as $key_f => $value_f) {
        // Find .src file
        if (!strcmp(substr($value_f, strpos($value_f, '.') + 1), 'src')) {
            $test_count++;
            $dir_count++;

            $file_name = substr($value_f, 0, strpos($value_f, '.'));

            // Remove last . from directory
            $actual_dir = substr($value_d, 0, strlen($value_d) - 1);

            if (!in_array($file_name.'.rc', $file_array)) {
                // Create rc file with 0
                file_put_contents($actual_dir.$file_name.'.rc', "0\n");
            }
            if (!in_array($file_name.'.in', $file_array)) {
                // Create rc file with 0
                file_put_contents($actual_dir.$file_name.'.in', "");
            }
            if (!in_array($file_name.'.out', $file_array)) {
                // Create rc file with 0
                file_put_contents($actual_dir.$file_name.'.out', "");
            }

            $returned_code;
            $output;

            // Exec command
            $command = 'php56 '.$parse_file.' < '.$actual_dir.$file_name.'.src'.' 2> /dev/null';

            exec($command, $output, $returned_code);

            // If error in parse.php occured
            if ($returned_code != 0) {
                $rc_file = fopen($actual_dir.$file_name.'.rc', 'r');
                $rc = fread($rc_file, 2);
                fclose($rc_file);

                if (!strcmp($rc, $returned_code)) {
                    $dir_success++;
                    $success_count++;
                    print("<li> Testovací soubor: ".$file_name." - <span style=\"font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: green;font-weight: bold;\">Úspěch</span> </li>");
                }
                else
                    print("<li> Testovací soubor: ".$file_name." - <span style=\"font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: red;font-weight: bold;\">Neúspěch</span>&emsp;&hArr; Rozdílný návratový kód od parse.php</li>");
            }
            else {
                // Create file for XML
                $tmp_f = tmpfile();
                $tmp_f_uri = stream_get_meta_data($tmp_f);
                file_put_contents($tmp_f_uri['uri'], $output);

                $command = "python3.6 ".$int_file." --source=".$tmp_f_uri['uri'].' < '.$actual_dir.$file_name.'.in'.' 2> /dev/null';

                unset($output);
                exec($command, $output, $returned_code);

                // If error in interpret.py occured
                if ($returned_code != 0) {
                    $rc_file = fopen($actual_dir.$file_name.'.rc', 'r');
                    $rc = fread($rc_file, 2);
                    fclose($rc_file);

                    if (!strcmp($rc, $returned_code)) {
                        $dir_success++;
                        $success_count++;
                        print("<li> Testovací soubor: ".$file_name." - <span style=\"font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: green;font-weight: bold;\">Úspěch</span> </li>");
                    }
                    else
                        print("<li> Testovací soubor: ".$file_name." - <span style=\"font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: red;font-weight: bold;\">Neúspěch</span>&emsp;&hArr; Rozdílný návratový kód od interpret.py</li>");
                }
                // Check for diff
                else {
                    file_put_contents($tmp_f_uri['uri'], $output);
                    unset($output);
                    $command = "diff ".$actual_dir.$file_name.'.out '.$tmp_f_uri['uri'];
                    exec($command, $output, $returned_code);

                    if (!isset($output[0])) {
                        $dir_success++;
                        $success_count++;
                        print("<li> Testovací soubor: ".$file_name." - <span style=\"font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: green;font-weight: bold;\">Úspěch</span> </li>");
                    }
                    else
                        print("<li> Testovací soubor: ".$file_name." - <span style=\"font-size: 22pt;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;color: red;font-weight: bold;\">Neúspěch</span>&emsp;&hArr; Rozdílný výstup od interpret.py</li>");
                }
                fclose($tmp_f);
            }
        }
    }

    print("</ol>\n<hr>
        </div>\n
        <div style=\"height: 40px;font-size: 22pt;font-weight: bold;\">&emsp;&emsp;Úspěšnost testů z daného adresáře: ".$dir_success."/".$dir_count.".</div><hr style=\"padding: 0px; margin: -1px; \">"); 
}

print("<div style=\"border:5px solid black;height:81px;line-height:81px;vertical-align: middle;background-color: PaleGreen;text-align: center;font-size: 30pt;font-weight: bold;\"> Celková úspěšnost všech testů: ".$success_count."/".$test_count.".</div>
        </body>\n</html>");

?>
