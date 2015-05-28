all: systemHallvmec15.database scaler.database chl.database \
	scaler_gen.database hbeat.database

#hms_mag.database: hms.sr
#	rm -f $@
#	../base/bin/hp700/atdb default.dctsdr $@ < hms.sr

systemHallvmec15.database: systemHallvmec15.sf
	rm -f $@
	../base/bin/hp700/atdb default.dctsdr $@ < $<

#scaler.database: scaler.db default.dctsdr default.sdrSum
#	./db2database -v scaler.db > z.db

scaler.database: scaler.sf
	rm -f $@
	../base/bin/hp700/atdb default.dctsdr $@ < $<

scaler_gen.database: scaler_gen.sf
	rm -f $@
	../base/bin/hp700/atdb default.dctsdr $@ < $<

chl.database: chl.sr
	rm -f $@
	../base/bin/hp700/atdb default.dctsdr $@ < $<

hbeat.database: hbeat.sr
	rm -f $@
	../base/bin/hp700/atdb default.dctsdr $@ < $<


