#!/usr/bin/perl
    $gencmd = "opendds_idl -Sa -St --no-dcps-data-type-warnings sample.idl";
    system($gencmd);
    system("mv sampleTypeSupportImpl.cpp pass1");
    system($gencmd);
    $result = system("diff pass1 sampleTypeSupportImpl.cpp");
    system("rm pass1 sampleT*");
    if ($result == "")
    {
        print "test PASSED\n";
        exit 0;
    }

    print "test FAILED\n";
    exit 1;

