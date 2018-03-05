<?php
/************************CONSTANTS**************************/
const ARG_ERR = 10;
const SYN_ERR = 21;
const OK_VAL = 0;
const STAT_TEST = 1;
const OUT_TEST = 0;

XMLDiff\File extends XMLDiff\Base {
/* Methods */
public string diff ( string $from , string $to )
public string merge ( string $src , string $diff )
/* Inherited methods */
public XMLDiff\Base::__construct ( string $nsname )
abstract public mixed XMLDiff\Base::diff ( mixed $from , mixed $to )
abstract public mixed XMLDiff\Base::merge ( mixed $src , mixed $diff )
}
/****************************ARGUMENTS TESTS***************************/

// command, expected return value, 0 if output, 1 if stats, file
$tests = array(
    array ('php56 parse.php < ./tests/source_files/00.in > ./tests/outputs/00.out', OK_VAL, OUT_TEST, "00.out"), 
    array ('php56 parse.php --stats=./tests/stats/01.stat --comments < ./tests/source_files/01.in> ./tests/outputs/01.out', OK_VAL, STAT_TEST, "01.stat"), 
    array ('php56 parse.php --comments --stats=./tests/stats/02.stat  < ./tests/source_files/02.in> ./tests/outputs/02.out', OK_VAL, STAT_TEST, "02.stat"), 
    array ('php56 parse.php --stats=./tests/stats/03.stat --loc < ./tests/source_files/03.in > ./tests/outputs/03.out', OK_VAL, STAT_TEST, "03.stat"), 
    array ('php56 parse.php --stats=./tests/stats/04.stat --comments --loc < ./tests/source_files/04.in > ./tests/outputs/04.out', OK_VAL, STAT_TEST, "04.stat"), 
    array ('php56 parse.php --stats=./tests/stats/05.stat --loc --comments < ./tests/source_files/05.in > ./tests/outputs/05.out', OK_VAL, STAT_TEST, "05.stat"),
    // Invalid arguments
    array ('php56 parse.php --loc --comments < ./tests/source_files/06.in > ./tests/outputs/06.out', ARG_ERR, OUT_TEST, "06.out"),
    array ('php56 parse.php --loc --help < ./tests/source_files/07.in > ./tests/outputs/07.out', ARG_ERR, OUT_TEST, "07.out"),
    array ('php56 parse.php --loc < ./tests/source_files/08.in > ./tests/outputs/08.out', ARG_ERR, OUT_TEST, "08.out"),
    array ('php56 parse.php --loc --comments --comments --stats=./tests/stats < ./tests/source_files/09.in > ./tests/outputs/09.out', ARG_ERR, OUT_TEST, "09.out"),
    array ('php56 parse.php --stats=./tests/stats --help< ./tests/source_files/10.in > ./tests/outputs/10.out', ARG_ERR, OUT_TEST, "10.out"),
    array ('php56 parse.php --stats=./tests/stats --comments --loc --something < ./tests/source_files/11.in > ./tests/outputs/11.out', ARG_ERR, OUT_TEST, "11.out"),
    // Spaces and tabs
    array ('php56 parse.php < ./tests/source_files/12.in > ./tests/outputs/12.out', OK_VAL, OUT_TEST, "12.out"),
    // UTF-8
    array ('php56 parse.php < ./tests/source_files/13.in > ./tests/outputs/13.out', OK_VAL, OUT_TEST, "13.out")
);

$out;
$ret_val;
$dxml = new XMLDiff\File;


foreach ($tests as $key => $value) {
    // Hide stderr
    exec($value[0].' 2> /dev/null', $out, $ret_val);
    if ($ret_val == $value[1]) {
        if ($value[2] == OUT_TEST) {
            //exec('xmllint --c14n ')
            $str = $dxml->diff('./tests/examples/'.$value[3], './tests/outputs/'.$value[3]);
            //exec('diff ./tests/examples/'.$value[3].' ./tests/outputs/'.$value[3], $out, $ret_val);
            //exec('rm -f ./tests/out');
        }
        else {
            $str = $dxml->diff('./tests/examples/'.$value[3], './tests/stats/'.$value[3]);
            //exec('diff ./tests/examples/'.$value[3].' ./tests/stats/'.$value[3], $out, $ret_val);
            //exec('rm -f ./tests/stats');
        }

        if (!isset($str))
            print("Test ".($key + 1)." OK!\n");
        else
            print("Test ".($key + 1)." FAIL! ---> diff\n");
    }
    else
        print("Test ".($key + 1)." FAIL! ---> return value\n");

    unset($out);
}

?>