<?php
/**
*  Project:     Simple PHP script testing functionality of parse.php
*  File:        test.php
*
*  Authors: Tomáš Nereča : xnerec00
*           Samuel Obuch : xobuch00
*   
*  !!!INFO!!!
*  Tests for parse.php .
*  Copy folder 'tests'  where your 'parse.php' is placed and execute test.php
*  Java is required to run jExamlXML
*
*/

/************************CONSTANTS**************************/
const ARG_ERR = 10;
const SYN_ERR = 21;
const OK_VAL = 0;

$GREEN = "\033[32m";
$RED = "\033[31m";
$COLOR_END = "\033[0m";

/***********************ARGUMENT PARSING********************/
if ($argc == 2) {
    if (!strcmp($argv[1], "--help") || !strcmp($argv[1], "-h")) {
        print("Tests for parse.php .\n".
              "Copy folder 'tests'  where your 'parse.php' is placed and execute test.php .\n".
              "Java is required to run jExamlXML !\n".
              "Arguments: [--help] [--clean]\n");
        exit(0);
    }
    else if (!strcmp($argv[1], "--clean") || !strcmp($argv[1], "-c")) {
        // Clean files
        exec('rm -f ./diff/*', $output, $ret_val);
        exec('rm -f ./my_output/*', $output, $ret_val);
        exit(0);
    }
    else {
        print("Invalid arguments. Try --help !\n");
        exit(1);
    }
}
else if ($argc != 1) {
    print("Invalid arguments. Try --help !\n");
    exit(1);
}

// command, expected return value, 0 if output, 1 if stats, file
$tests = array(
    // Basic
    array (PHP_BINARY.' ../parse.php < ./source/00.in > ./my_output/00.out', OK_VAL,  "00.out"),
    // STATS
    array (PHP_BINARY.' ../parse.php --stats=./my_output/01.stat --comments < ./source/01.in> ./my_output/01.out', OK_VAL,  "01.stat"), 
    array (PHP_BINARY.' ../parse.php --comments --stats=./my_output/02.stat  < ./source/02.in> ./my_output/02.out', OK_VAL,  "02.stat"), 
    array (PHP_BINARY.' ../parse.php --stats=./my_output/03.stat --loc < ./source/03.in > ./my_output/03.out', OK_VAL,  "03.stat"), 
    array (PHP_BINARY.' ../parse.php --stats=./my_output/04.stat --comments --loc < ./source/04.in > ./my_output/04.out', OK_VAL,  "04.stat"), 
    array (PHP_BINARY.' ../parse.php --stats=./my_output/05.stat --loc --comments < ./source/05.in > ./my_output/05.out', OK_VAL,  "05.stat"),
    // Invalid arguments
    array (PHP_BINARY.' ../parse.php --loc --comments < ./source/06.in > ./my_output/06.out', ARG_ERR,  "06.out"),
    array (PHP_BINARY.' ../parse.php --loc --help < ./source/07.in > ./my_output/07.out', ARG_ERR,  "07.out"),
    array (PHP_BINARY.' ../parse.php --loc < ./source/08.in > ./my_output/08.out', ARG_ERR,  "08.out"),
    array (PHP_BINARY.' ../parse.php --loc --comments --comments --stats=./my_output < ./source/09.in > ./my_output/09.out', ARG_ERR,  "09.out"),
    array (PHP_BINARY.' ../parse.php --stats=./my_output --help< ./source/10.in > ./my_output/10.out', ARG_ERR,  "10.out"),
    array (PHP_BINARY.' ../parse.php --stats=./my_output --comments --loc --something < ./source/11.in > ./my_output/11.out', ARG_ERR,  "11.out"),
    // Spaces and tabs
    array (PHP_BINARY.' ../parse.php < ./source/12.in > ./my_output/12.out', OK_VAL,  "12.out"),
    // UTF-8
    array (PHP_BINARY.' ../parse.php < ./source/13.in > ./my_output/13.out', OK_VAL,  "13.out"),
    // * & ...
    array (PHP_BINARY.' ../parse.php < ./source/14.in > ./my_output/14.out', OK_VAL,  "14.out"),
    // Google drive Chadima tests
    array (PHP_BINARY.' ../parse.php < ./source/15.in > ./my_output/15.out', OK_VAL,  "15.out"),
    array (PHP_BINARY.' ../parse.php < ./source/16.in > ./my_output/16.out', OK_VAL,  "16.out"),
    array (PHP_BINARY.' ../parse.php < ./source/17.in > ./my_output/17.out', OK_VAL,  "17.out"),
    array (PHP_BINARY.' ../parse.php < ./source/18.in > ./my_output/18.out', OK_VAL,  "18.out"),
    array (PHP_BINARY.' ../parse.php < ./source/19.in > ./my_output/19.out', OK_VAL,  "19.out"),
    array (PHP_BINARY.' ../parse.php < ./source/20.in > ./my_output/20.out', OK_VAL,  "20.out"),
    array (PHP_BINARY.' ../parse.php < ./source/21.in > ./my_output/21.out', OK_VAL,  "21.out"),
    array (PHP_BINARY.' ../parse.php < ./source/22.in > ./my_output/22.out', OK_VAL,  "22.out"),
    array (PHP_BINARY.' ../parse.php < ./source/23.in > ./my_output/23.out', OK_VAL,  "23.out"),
    // Comment in first line
    array (PHP_BINARY.' ../parse.php < ./source/24.in > ./my_output/24.out', OK_VAL,  "24.out"),
    // Syntactic errors
    array (PHP_BINARY.' ../parse.php < ./source/25.in > ./my_output/25.out', SYN_ERR, "25.out"),
    array (PHP_BINARY.' ../parse.php < ./source/26.in > ./my_output/26.out', SYN_ERR, "26.out"),
    // Only comments and white
    array (PHP_BINARY.' ../parse.php < ./source/27.in > ./my_output/27.out', OK_VAL, "27.out"),
    // lF
    array (PHP_BINARY.' ../parse.php < ./source/28.in > ./my_output/28.out', SYN_ERR, "28.out"),
    // Escape sequence
    array (PHP_BINARY.' ../parse.php < ./source/29.in > ./my_output/29.out', OK_VAL, "29.out"),
    // Operands on multiple lines
    array (PHP_BINARY.' ../parse.php < ./source/30.in > ./my_output/30.out', SYN_ERR, "30.out"),
    array (PHP_BINARY.' ../parse.php < ./source/31.in > ./my_output/31.out', SYN_ERR, "31.out"),
    // Empty string
    array (PHP_BINARY.' ../parse.php < ./source/32.in > ./my_output/32.out', OK_VAL, "32.out"),
    // Wrong label name
    array (PHP_BINARY.' ../parse.php < ./source/33.in > ./my_output/33.out', SYN_ERR, "33.out"),
    // Wrong variable name
    array (PHP_BINARY.' ../parse.php < ./source/34.in > ./my_output/34.out', SYN_ERR, "34.out"),
    // Weird CONCAT operands but OK
    array (PHP_BINARY.' ../parse.php < ./source/35.in > ./my_output/35.out', OK_VAL, "35.out"),
    // Empty int
    array (PHP_BINARY.' ../parse.php < ./source/36.in > ./my_output/36.out', SYN_ERR, "36.out"),
    // Vondracek gitlab
    array (PHP_BINARY.' ../parse.php < ./source/37.in > ./my_output/37.out', SYN_ERR, "37.out"),
    array (PHP_BINARY.' ../parse.php < ./source/38.in > ./my_output/38.out', SYN_ERR, "38.out"),
    array (PHP_BINARY.' ../parse.php < ./source/39.in > ./my_output/39.out', SYN_ERR, "39.out"),
    array (PHP_BINARY.' ../parse.php < ./source/40.in > ./my_output/40.out', SYN_ERR, "40.out"),
    array (PHP_BINARY.' ../parse.php < ./source/41.in > ./my_output/41.out', SYN_ERR, "41.out"),
    array (PHP_BINARY.' ../parse.php < ./source/42.in > ./my_output/42.out', SYN_ERR, "42.out"),
    array (PHP_BINARY.' ../parse.php < ./source/43.in > ./my_output/43.out', SYN_ERR, "43.out"),
    array (PHP_BINARY.' ../parse.php < ./source/44.in > ./my_output/44.out', SYN_ERR, "44.out"),
    array (PHP_BINARY.' ../parse.php < ./source/45.in > ./my_output/45.out', SYN_ERR, "45.out"),
    array (PHP_BINARY.' ../parse.php < ./source/46.in > ./my_output/46.out', SYN_ERR, "46.out"),
    array (PHP_BINARY.' ../parse.php < ./source/47.in > ./my_output/47.out', SYN_ERR, "47.out"),
    array (PHP_BINARY.' ../parse.php < ./source/48.in > ./my_output/48.out', SYN_ERR, "48.out"),
    array (PHP_BINARY.' ../parse.php < ./source/49.in > ./my_output/49.out', SYN_ERR, "49.out")
);

$output;
$command;
$ret_val;

// Clean before executing
exec('rm -f ./diff/*', $output, $ret_val);
exec('rm -f ./my_output/*', $output, $ret_val);

// Testing
foreach ($tests as $key => $value) {
    unset($output);
    // 2> /dev/null - hide stderr
    exec($value[0].' 2> /dev/null', $output, $ret_val);

    if ($ret_val == $value[1]) {
        if($ret_val != 0) {
            print(" Test ".($key)."$GREEN OK! $COLOR_END \n");
            continue;
        }

        if (!strcmp(substr($value[2], 3), "out")) {
            $command = 'java -jar jexamxml.jar ./ref_output/'.$value[2].' ./my_output/'.$value[2].' ./diff/'.substr($value[2], 0, 2).'.diff options';
            exec($command, $output);

            if (!strcmp("Two files are identical", $output[2]))
                print(" Test ".($key)."$GREEN OK! $COLOR_END \n");
            else {
                print(" Test ".($key)."$RED FAIL! ---> diff $COLOR_END \n");
            }
        }
        else {
            $command = 'diff ./ref_output/'.$value[2].' ./my_output/'.$value[2];
            exec($command, $output);

            if (!isset($output[0]))
                print(" Test ".($key)."$GREEN OK! $COLOR_END \n");
            else
                print(" Test ".($key)."$RED FAIL! ---> diff $COLOR_END \n");
        }
    }
    else {
        print(" Test ".($key)."$RED FAIL! ---> Expected return value: ".$value[1]."\n");
        print(" Your return value: ".$ret_val." $COLOR_END\n");
    }
}

/********************************************PERFORMANCE********************************************/

print("Normal performance test:\n");
$time_pre = microtime(true);
exec(PHP_BINARY.' ../parse.php < ./source/perf_1000.in > ./my_output/perf_1000.out 2> /dev/null', $output, $ret_val);
$time_post = microtime(true);

if ($ret_val != 0) {
    print(" Performance test $RED FAIL! ---> Expected return value: 0\n");
    print(" Your return value: ".$ret_val." $COLOR_END\n");
}
print("Execution time: ".($time_post - $time_pre)."s\n");

print("Extreme performance test:\n");
$time_pre = microtime(true);
exec(PHP_BINARY.' ../parse.php < ./source/perf_10000.in > ./my_output/perf_10000.out 2> /dev/null', $output, $ret_val);
$time_post = microtime(true);

if ($ret_val != 0) {
    print(" Performance test $RED FAIL! ---> Expected return value: 0\n");
    print(" Your return value: ".$ret_val." $COLOR_END\n");
}
print("Execution time: ".($time_post - $time_pre)."s\n");

?>
