<?php
/**
*  Course:      Principles of Programming Languages (IPP)
*  Project:     Implementation of the IPPcode18 imperative language interpreter
*  File:        test.php
*
*  Author: Tomáš Nereča : xnerec00
*   
*  !!!INFO!!!
*  Tests for parse.php actually.
*  Copy folder 'tests'  where your 'parse.php' is placed and execute test.php
*  Java is required to run jExamlXML
*
*/

/************************CONSTANTS**************************/
const ARG_ERR = 10;
const SYN_ERR = 21;
const OK_VAL = 0;

/****************************ARGUMENTS TESTS***************************/

if ($argc == 2) {
    if (!strcmp($argv[1], "--help") || !strcmp($argv[1], "-h")) {
        print("Tests for parse.php .\n".
              "Copy folder 'tests'  where your 'parse.php' is placed and execute test.php .\n".
              "Java is required to run jExamlXML !\n");
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

// command, expected return value and reference output
$tests = array(
    array (PHP_BINARY.' ../parse.php < ./source/00.in > ./my_output/00.out', OK_VAL,  "00.out"), 
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
    // FB tests
    array (PHP_BINARY.' ../parse.php < ./source/15.in > ./my_output/15.out', OK_VAL,  "15.out"),
    array (PHP_BINARY.' ../parse.php < ./source/16.in > ./my_output/16.out', OK_VAL,  "16.out"),
    array (PHP_BINARY.' ../parse.php < ./source/17.in > ./my_output/17.out', OK_VAL,  "17.out"),
    array (PHP_BINARY.' ../parse.php < ./source/18.in > ./my_output/18.out', OK_VAL,  "18.out"),
    array (PHP_BINARY.' ../parse.php < ./source/19.in > ./my_output/19.out', OK_VAL,  "19.out"),
    array (PHP_BINARY.' ../parse.php < ./source/20.in > ./my_output/20.out', OK_VAL,  "20.out"),
    array (PHP_BINARY.' ../parse.php < ./source/21.in > ./my_output/21.out', OK_VAL,  "21.out"),
    array (PHP_BINARY.' ../parse.php < ./source/22.in > ./my_output/22.out', OK_VAL,  "22.out"),
    array (PHP_BINARY.' ../parse.php < ./source/23.in > ./my_output/23.out', OK_VAL,  "23.out")
);

$output;
$command;
$ret_val;

foreach ($tests as $key => $value) {
    // Hide stderr
    exec($value[0].' 2> /dev/null', $out, $ret_val);

    if ($ret_val == 1) {
        print("Something went wrong. Try --help !\n");
        exit(1);
    }

    if ($ret_val == $value[1]) {
        $command = 'java -jar jexamxml.jar ./ref_output/'.$value[2].' ./my_output/'.$value[2];
        exec($command, $output);

        if (!strcmp("Two files are identical", $output[2]))
            print("Test ".($key + 1)." OK!\n");
        else
            print("Test ".($key + 1)." FAIL! ---> diff\n");
    }
    else
        print("Test ".($key + 1)." FAIL! ---> return value".$ret_val."\n");

    unset($out);
}

?>
