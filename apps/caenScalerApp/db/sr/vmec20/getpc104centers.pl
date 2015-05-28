#!/usr/bin/perl

@hcpc10401 = (
	      "hks:kq1_mps_i_set",
	      "hks:kq1_mps_i_out",
	      "hks:kq1_mps_v_out",
	      "hks:kq1_mps_t",
	      "hks:eq2_mps_i_set",
	      "hks:eq2_mps_i_out",
	      "hks:eq2_mps_v_out",
	      "hks:eq2_mps_t"
	      );

@hcpc10402 = (
	      "hks:spl_hp_field",
	      "hks:kd_nmr_field",
	      "hks:ed_nmr_field",
	      "hks:kq1_hp_field",
	      "hks:kq1_hp_temp",
	      "hks:kq2_hp_field",
	      "hks:kd_mps_i_set",
	      "hks:kd_mps_i_out",
	      "hks:ed_mps_i_set",
	      "hks:ed_mps_i_out"
	      );
	      
$caget = "caget ";

foreach $pv (@hcpc10401) {
    $caget .= " ".$pv;
}

open(CAGET, $caget."|");
print "%hcpc10401_list = (\n";
while(<CAGET>) {
    chomp;
    ($pv_name, $value) = /(\S*)\s*(\S*)/;
    print qq{\t     "$pv_name",$value,\n};
}
print "\t     );\n";

$caget = "caget ";

foreach $pv (@hcpc10402) {
    $caget .= " ".$pv;
}

open(CAGET, $caget."|");
print "%hcpc10402_list = (\n";
while(<CAGET>) {
    chomp;
    ($pv_name, $value) = /(\S*)\s*(\S*)/;
    print qq{\t     "$pv_name",$value,\n};
}
print "\t     );\n";

