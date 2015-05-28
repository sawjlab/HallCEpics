#!/usr/bin/perl

%bpm_list = (
	     "IPM3C07.XPOS",0.0,
	     "IPM3C08.XPOS",0.0,
	     "IPM3C12.XPOS",0.0,
	     "IPM3C16.XPOS",0.0,
	     "IPM3C17.XRAW",0.0,
	     "IPM3C17.XPOS",0.0,
	     "IPM3C18.XPOS",0.0,
	     "IPM3H01.XRAW",0.0,
	     "IPM3H02A.XRAW",0.0,
	     "IPM3H02B.XRAW",0.0,
	     "IPM3H02C.XRAW",0.0,
	     "IPM3H03A.XRAW",0.0,
	     "IPM3H03A.YRAW",0.0,
	     "IPM3H03B.XRAW",0.0,
	     "IPM3H03B.YRAW",0.0
	     );

$caget = "caget ";

foreach $bpm_name (sort keys %bpm_list) {
    $caget .= " ".$bpm_name;
}

open(CAGET, $caget."|");
print "%bpm_list = (\n";
while(<CAGET>) {
    chomp;
    ($bpm_name, $value) = /(\S*)\s*(\S*)/;
    print qq{\t     "$bpm_name",$value,\n};
}
print "\t     );\n";

