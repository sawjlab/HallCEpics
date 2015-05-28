#!/usr/bin/perl

# make_hks_hw.pl

# Make the database hw file for mirroring a bunch of beamline epics variables
# so that we can set alarams on them.
$HWFILE="part_hks09_auto.hw";
$bpmwindow1=0.100; 		# +/- this for LOW and HIGH
$bpmwindow2=0.200;		# +/- this flr LOLO and HIHI

$ewindow1=0.000043; 		# Fractional window
$ewindow2=0.000054;		# Fractional window

$magwindow1=0.0002;		# Fractional window
$magwindow2=0.0003;		# Fractional window

$magdwwindow1=0.0019;		# Fractional window
$magdwwindow2=0.0030;		# Fractional window

$prefix="hks";

# Values grabbed at 9/22/09 11:47

%bpm_list = (
             "IPM3C07.XPOS",0.00954881,
             "IPM3C08.XPOS",0.0424699,
             "IPM3C12.XPOS",-0.0049842,
             "IPM3C16.XPOS",0.0091058,
             "IPM3C17.XPOS",-0.365695,
             "IPM3C17.XRAW",-0.142695,
             "IPM3C18.XPOS",-0.143202,
             "IPM3H01.XRAW",-0.78503,
             "IPM3H02A.XRAW",0.28461,
             "IPM3H02B.XRAW",0.447843,
             "IPM3H02C.XRAW",0.603376,
             "IPM3H03A.XRAW",-1.67996,
             "IPM3H03A.YRAW",-2.00179,
             "IPM3H03B.XRAW",-6.16312,
             "IPM3H03B.YRAW",-2.40187,
             );

%mag_list = (
	     "MDZ3H01M",314.923,
	     "MFZ3H03M",417.502,
	     "MDW3H04AHM",125.635,
	     "MSP3H04M",953.971,
	     "HALLC:p",2344.09,
	     "HCNMR:SIG",0.192096,
	     "MHKSQ2M",291.987,
	     "MHESQ1M",506.983
	     );

# Also need HKS Q2, HES Q1
	     
open(HW,">$HWFILE");

$window1=$bpmwindow1;
$window2=$bpmwindow2;

foreach $bpm_name (sort keys %bpm_list) {
    $center = $bpm_list{$bpm_name};
    $newPV = $prefix.$bpm_name;
    $newPV =~ s/\.//;
    print HW "PV: $newPV \tType: ai ()\n";
    print HW "\n";
    print HW "DESC $bpm_name copy\n";
    print HW "INP $bpm_name CPP\n";
#    print HW "SCAN Passive\n";
    print HW "SCAN Passive\n";
    print HW "DTYP Soft Channel\n";
    $tmp = $center+$window2;
    print HW "HIHI $tmp\n";
    $tmp = $center+$window1;
    print HW "HIGH $tmp\n";
    $tmp = $center-$window1;
    print HW "LOW $tmp\n";
    $tmp = $center-$window2;
    print HW "LOLO $tmp\n";
    print HW "HHSV MAJOR\n";
    print HW "HSV MINOR\n";
    print HW "LSV MINOR\n";
    print HW "LLSV MAJOR\n";
    print HW "PREC 4\n";
    print HW "\n\f\n";
}

foreach $mag_name (keys %mag_list) {
    $center = $mag_list{$mag_name};
    $newPV = $prefix.$mag_name;
    print HW "PV: $newPV \tType: ai ()\n";
    print HW "\n";
    print HW "DESC $mag_name copy\n";
    print HW "INP $mag_name CPP\n";
    print HW "DTYP Soft Channel\n";
    print HW "PREC 7\n";
    print HW "SCAN Passive\n";
    if($mag_name eq "HALLC:p" or $mag_name eq "HCNMR:SIG") {
	$window1 = $ewindow1;
	$window2 = $ewindow2;
    } elsif ($mag_name eq "MDW3H04AHM") {
	$window1 = $magdwwindow1;
	$window2 = $magdwwindow2;
    } else {
	$window1 = $magwindow1;
	$window2 = $magwindow2;
    }
    $tmp = $center+$center*$window2;
    print HW "HIHI $tmp\n";
    $tmp = $center+$center*$window1;
    print HW "HIGH $tmp\n";
    $tmp = $center-$center*$window1;
    print HW "LOW $tmp\n";
    $tmp = $center-$center*$window2;
    print HW "LOLO $tmp\n";
    print HW "HHSV MAJOR\n";
    print HW "HSV MINOR\n";
    print HW "LSV MINOR\n";
    print HW "LLSV MAJOR\n";
    print HW "\n\f\n";
}
  
close(HW);
print "Database definition file: $HWFILE\n";
