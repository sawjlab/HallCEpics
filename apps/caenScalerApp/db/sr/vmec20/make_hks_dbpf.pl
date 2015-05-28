#!/usr/bin/perl

# make_hks_dbpf.pl

# Create limits for all the variables on PC104 boards

# Don't forget to do limits on negative values correctly

# Splitter field Minor/Major  0.000075  0.000100

# KD     1.4403657  - 1.4403677                0.00001   0.000015

# ED     1.174470 - 1.174474   0.00001   0.000015

# KQ1   -.6733 -6732                 0.000075  0.0001

# KQ2   .3942 - .3943         0.0013  0.0017

# EQ1    1.184575 - 1.18464                     

# EQ2    1.21345 - 1.21355

$PC10401FILE="hcpc10401_limits.boot";
$PC10402FILE="hcpc10402_limits.boot";
$CAPUTFILE="caput.sh";

$temp_window1=1.0;		# Absolute
$temp_window2=2.0;

$volt_window1=1.0;		# Absolute
$volt_window2=2.0;

$noisy_volt_window1=18.0;
$noisy_volt_window2=25.0;

$field_window1=0.00001;		# Fraction
$field_window2=0.000015;

$hp_window1=0.0004;		# Fraction
$hp_window2=0.0006;

$current_window1=0.000075;
$current_window2=0.0001;

$noisy_current_window1=0.0030;
$noisy_current_window2=0.0040;

$prefix="hks";

%hcpc10401_list = (
             "hks:kq1_mps_i_set",407.6,
             "hks:kq1_mps_i_out",407.02,
             "hks:kq1_mps_v_out",68,
             "hks:kq1_mps_t",31.1,
             "hks:eq2_mps_i_set",578.419,
             "hks:eq2_mps_i_out",576.09,
             "hks:eq2_mps_v_out",347.5,	# Very noisy
             "hks:eq2_mps_t",31.7,
             );
%hcpc10402_list = (
		   "hks:spl_hp_field",-0.360756,
		   "hks:kd_nmr_field",1.4396,
		   "hks:ed_nmr_field",1.17448,
		   "hks:kq1_hp_field",-0.673225,	# 67325 67320 toggle
		   # Needs +/- .000035
		   "hks:kq1_hp_temp",28.7,
		   "hks:kq2_hp_field",-0.39415,   #3940 3943 toggle
		   # Needs +/- .00038
		   "hks:eq1_hp_field",1.1854100,
		   "hks:eq1_hp_temp",28.11,
		   "hks:eq2_hp_field",1.213081,	# 1.2130760 1.2130860 range
		   # Needs +/- 
		   "hks:eq2_hp_temp",-36.41,
		   "hks:kd_mps_i_set",1058.3,
		   "hks:kd_mps_i_out",1060.14,
		   "hks:ed_mps_i_set",675.63,
		   "hks:ed_mps_i_out",675.991,
		   );
open(CA,">$CAPUTFILE");
open(OUT,">$PC10401FILE");
print CA "#!/bin/sh\n";
print CA qq{export EPICS_CA_ADDR_LIST="129.57.171.255 129.57.255.13"\n};
foreach $sig_name (sort keys %hcpc10401_list) {
    $center = $hcpc10401_list{$sig_name};
    if($sig_name =~ /_t$/ || $sig_name =~ /_temp$/) {
	$high = $center + $temp_window1;
	$low = $center - $temp_window1;
	$hihi = $center + $temp_window2;
	$lolo = $center - $temp_window2;
    } elsif ($sig_name =~ /_v_out$/) {
	if ($sig_name eq "hks:eq2_mps_v_out") {
	    $win1 = $noisy_volt_window1;
	    $win2 = $noisy_volt_window2;
	} else { 
	    $win1 = $volt_window1;
	    $win2 = $volt_window2;
	}
	$high = $center + $win1;
	$low = $center - $win1;
	$hihi = $center + $win2;
	$lolo = $center - $win2;
    } elsif ($sig_name =~ /_i_set$/ || $sig_name=~/_i_out/) {
	if ($sig_name eq "hks:eq2_mps_i_out" or $sig_name eq "hks:kq1_mps_i_out") {
	    $win1 = $noisy_current_window1;
	    $win2 = $noisy_current_window2;
	} else { 
	    $win1 = $current_window1;
	    $win2 = $current_window2;
	}
	$high = $center*(1.0+$win1);
	$low = $center*(1-$win1);
	$hihi = $center*(1+$win2);
	$lolo = $center*(1-$win2);
    }
    print OUT qq{dbpf "$sig_name.LOLO", "$lolo"\n};
    print OUT qq{dbpf "$sig_name.LOW", "$low"\n};
    print OUT qq{dbpf "$sig_name.HIGH", "$high"\n};
    print OUT qq{dbpf "$sig_name.HIHI", "$hihi"\n};
    print CA qq{caput $sig_name.LOLO $lolo\n};
    print CA qq{caput $sig_name.LOW $low\n};
    print CA qq{caput $sig_name.HIGH $high\n};
    print CA qq{caput $sig_name.HIHI $hihi\n};
    print OUT qq{dbpf "$sig_name.LLSV", "MAJOR"\n};
    print OUT qq{dbpf "$sig_name.LSV", "MINOR"\n};
    print OUT qq{dbpf "$sig_name.HSV", "MINOR"\n};
    print OUT qq{dbpf "$sig_name.HHSV", "MAJOR"\n};
    print CA qq{caput $sig_name.LLSV 2\n};
    print CA qq{caput $sig_name.LSV 1\n};
    print CA qq{caput $sig_name.HSV 1\n};
    print CA qq{caput $sig_name.HHSV 2\n};
}
close(OUT);


open(OUT,">$PC10402FILE");
foreach $sig_name (sort keys %hcpc10402_list) {
    $center = $hcpc10402_list{$sig_name};
    if($sig_name =~ /_t$/ || $sig_name =~ /_temp$/) {
	$high = $center + $temp_window1;
	$low = $center - $temp_window1;
	$hihi = $center + $temp_window2;
	$lolo = $center - $temp_window2;
    } elsif ($sig_name =~ /_v_out$/) {
	$high = $center + $volt_window1;
	$low = $center - $volt_window1;
	$hihi = $center + $volt_window2;
	$lolo = $center - $volt_window2;
    } elsif ($signame =~ /_i_(set|out)$/) {
	$high = $center*(1.0+$current_window1);
	$low = $center*(1 - $current_window1);
	$hihi = $center*(1+$current_window2);
	$lolo = $center*(1-$current_window2);
    } elsif ($signame =~ /_nmr_field$/) {
	$high = $center+abs($center)*$field_window1;
	$low = $center-abs($center)*$field_window1;
	$hihi = $center+abs($center)*$field_window2;
	$lolo = $center-abs($center)*$field_window2;
    } else {
	$high = $center+abs($center)*$hp_window1;
	$low = $center-abs($center)*$hp_window1;
	$hihi = $center+abs($center)*$hp_window2;
	$lolo = $center-abs($center)*$hp_window2;
    }	
    print OUT qq{dbpf "$sig_name.LOLO", "$lolo"\n};
    print OUT qq{dbpf "$sig_name.LOW", "$low"\n};
    print OUT qq{dbpf "$sig_name.HIGH", "$high"\n};
    print OUT qq{dbpf "$sig_name.HIHI", "$hihi"\n};
    print CA qq{caput $sig_name.LOLO $lolo\n};
    print CA qq{caput $sig_name.LOW $low\n};
    print CA qq{caput $sig_name.HIGH $high\n};
    print CA qq{caput $sig_name.HIHI $hihi\n};
    print OUT qq{dbpf "$sig_name.LLSV", "MAJOR"\n};
    print OUT qq{dbpf "$sig_name.LSV", "MINOR"\n};
    print OUT qq{dbpf "$sig_name.HSV", "MINOR"\n};
    print OUT qq{dbpf "$sig_name.HHSV", "MAJOR"\n};
    print CA qq{caput $sig_name.LLSV 2\n};
    print CA qq{caput $sig_name.LSV 1\n};
    print CA qq{caput $sig_name.HSV 1\n};
    print CA qq{caput $sig_name.HHSV 2\n};
}
close(OUT);
close(CA);
	
	     
