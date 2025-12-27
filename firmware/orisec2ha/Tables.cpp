#include "Tables.h"
#include "Globals.h"

void initTables() {
  for (int i=1;i<=MAX_ZONES_HARD;i++) {
    zones[i].present=false; zones[i].label=""; lastZoneState[i]=""; bootZoneStatus[i]=0x80;
  }
  for (int p=1;p<=MAX_PARTITIONS_HARD;p++) {
    parts[p].present=false; parts[p].label=""; lastPartAlarmState[p]="";
  }
  for (int o=1;o<=MAX_OUTPUTS_HARD;o++) {
    outs[o].present=false; outs[o].label=""; lastOutState[o]="";
  }
  partitionCount=0;
  outputCount=0;
  lastVolt0=lastVolt1=lastVolt2="";
}
