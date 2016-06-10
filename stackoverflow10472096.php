#! /usr/bin/php 
<?php

    $options = getopt("f:r:");
    $inputFile = $options['f'];
    $replacement = $options['r'];
    // read entire contents of input file 
    $inputFileContents = file_get_contents($inputFile);
    // setup the regex and execute the search
    $pattern = '/.*link.*href=["|\']?(.*[\\\|\/]?.*)\.css["|\']?.*/';
    preg_match_all($pattern, $inputFileContents, $matches);
    // remove last occurance of regex 
    // these are the lines we'll want to hang onto
    $matchedLines = $matches[0];
    array_pop($matchedLines);
    // isolate the last css file name
    $matchedFileName = array_pop($matches[1]);
    // first substitution replaces all lines with <link> with 
    // an empty string (deletes them)
    $inputFileContents = str_replace($matchedLines,'',$inputFileContents);
    // second substitution replaces the matched file name
    // with the desired string
    $inputFileContents = str_replace($matchedFileName,$replacement,$inputFileContents);
    //*/
      // save to new file for debugging
      $outputFileName = "output.html";
      $outputFile = fopen($outputFileName,'w+');
      fwrite($outputFile,$inputFileContents);
      fclose($outputFile);
    /*/
      // save changes to original file
      $origFile = fopen($inputFile,'w+');
      fwrite($origFile,$inputFileContents);
      fclose($origFile);
    //*/
    exit();
?>
