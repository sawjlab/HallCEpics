#!/bin/perl

$NCRATES=1;
$NSUBADD=5;

$BASE_NAME="HVM";
@atribs = ("fast","slow","drive","initial","temperature","temp_initial");

open(SF,">hvm.sf");

for($icrate=1;$icrate<=$NCRATES;$icrate++) {
    for($isub=1;$isub<=$NSUBADD;$isub++) {
	foreach $attribute (@atribs) {
	    $signame = "${BASE_NAME}_${attribute}_${icrate}_${isub}";
	    print "$signame\n";
	    print SF "PV: $signame     Type: ai\n";
	    print SF "DESC analog input record\n";
	    print SF "SCAN 2 second\n";
	    print SF "DTYP Soft Channel\n";
	    print SF "PREC 0\n";
	    print SF "\14\n";
	}
    }
}
