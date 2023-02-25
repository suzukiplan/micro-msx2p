#ifndef INCLUDE_TC8566AF_HPP
#define INCLUDE_TC8566AF_HPP

class TC8566AF {
private:
    static const int NUMBER_OF_DRIVES = 2;
    static const int NUMBER_OF_SIDES = 2;
    static const int NUMBER_OF_TRACKS = 80;
    static const int NUMBER_OF_SECTORS = 9;
    static const int SECTOR_SIZE = 512;
    static const int CMD_UNKNOWN = 0;
    static const int CMD_READ_DATA = 1;
    static const int CMD_WRITE_DATA = 2;
    static const int CMD_WRITE_DELETED_DATA = 3;
    static const int CMD_READ_DELETED_DATA = 4;
    static const int CMD_READ_DIAGNOSTIC = 5;
    static const int CMD_READ_ID = 6;
    static const int CMD_FORMAT = 7;
    static const int CMD_SCAN_EQUAL = 8;
    static const int CMD_SCAN_LOW_OR_EQUAL = 9;
    static const int CMD_SCAN_HIGH_OR_EQUAL = 10;
    static const int CMD_SEEK = 11;
    static const int CMD_RECALIBRATE = 12;
    static const int CMD_SENSE_INTERRUPT_STATUS = 13;
    static const int CMD_SPECIFY = 14;
    static const int CMD_SENSE_DEVICE_STATUS = 15;
    static const int PHASE_IDLE = 0;
    static const int PHASE_COMMAND = 1;
    static const int PHASE_DATATRANSFER = 2;
    static const int PHASE_RESULT = 3;
    static const int STM_DB0 = 0x01;
    static const int STM_DB1 = 0x02;
    static const int STM_DB2 = 0x04;
    static const int STM_DB3 = 0x08;
    static const int STM_CB = 0x10;
    static const int STM_NDM = 0x20;
    static const int STM_DIO = 0x40;
    static const int STM_RQM = 0x80;
    static const int ST0_DS0 = 0x01;
    static const int ST0_DS1 = 0x02;
    static const int ST0_HD = 0x04;
    static const int ST0_NR = 0x08;
    static const int ST0_EC = 0x10;
    static const int ST0_SE = 0x20;
    static const int ST0_IC0 = 0x40;
    static const int ST0_IC1 = 0x80;
    static const int ST1_MA = 0x01;
    static const int ST1_NW = 0x02;
    static const int ST1_ND = 0x04;
    static const int ST1_OR = 0x10;
    static const int ST1_DE = 0x20;
    static const int ST1_EN = 0x80;
    static const int ST2_MD = 0x01;
    static const int ST2_BC = 0x02;
    static const int ST2_SN = 0x04;
    static const int ST2_SH = 0x08;
    static const int ST2_NC = 0x10;
    static const int ST2_DD = 0x20;
    static const int ST2_CM = 0x40;
    static const int ST3_DS0 = 0x01;
    static const int ST3_DS1 = 0x02;
    static const int ST3_HD = 0x04;
    static const int ST3_2S = 0x08;
    static const int ST3_TK0 = 0x10;
    static const int ST3_RDY = 0x20;
    static const int ST3_WP = 0x40;
    static const int ST3_FLT = 0x80;
    
    struct DiskDrive {
        bool readOnly;
        int size;
        unsigned char sectors[8192][SECTOR_SIZE];
    } drives[NUMBER_OF_DRIVES];
    
    struct Callback {
        void* arg;
        void (*diskReadListener)(void* arg, int driveId, int sector);
        void (*diskWriteListener)(void* arg, int driveId, int sector);
    } CB;

public:
    struct Context {
        int phaseStep;
        int sectorOffset;
        unsigned char drive;
        unsigned char mainStatus;
        unsigned char status[4];
        unsigned char phase;
        unsigned char command;
        unsigned char commandCode;
        unsigned char fillerByte;
        unsigned char cylinderNumber;
        unsigned char side;
        unsigned char sectorNumber;
        unsigned char number;
        unsigned char currentTrack;
        unsigned char sectorsPerCylinder;
        unsigned char sectorBuf[SECTOR_SIZE];
    } ctx;
    
    TC8566AF() {
        memset(&this->drives, 0, sizeof(this->drives));
        memset(&this->CB, 0, sizeof(this->CB));
        this->reset();
    }
    
    void setDiskReadListener(void* arg, void (*diskReadListener)(void* arg, int driveId, int sector)) {
        this->CB.arg = arg;
        this->CB.diskReadListener = diskReadListener;
    }

    void setDiskWriteListener(void* arg, void (*diskWriteListener)(void* arg, int driveId, int sector)) {
        this->CB.arg = arg;
        this->CB.diskWriteListener = diskWriteListener;
    }

    void reset() {
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->ctx.mainStatus = STM_NDM | STM_RQM;
    }

    void ejectDisk(int driveId) {
        if (driveId < 0 || NUMBER_OF_DRIVES <= driveId) return;
        memset(&this->drives[driveId], 0, sizeof(struct DiskDrive));
    }

    void insertDisk(int driveId, const void* data, size_t size, bool readOnly) {
        if (driveId < 0 || NUMBER_OF_DRIVES <= driveId) return;
        this->ejectDisk(driveId);
        this->drives[driveId].readOnly = readOnly;
        const unsigned char* ptr = (const unsigned char*)data;
        int si = 0;
        while (0 < size) {
            if (size < SECTOR_SIZE) {
                memcpy(this->drives[driveId].sectors[si], ptr, size);
                size = 0;
            } else {
                memcpy(this->drives[driveId].sectors[si], ptr, SECTOR_SIZE);
                size -= SECTOR_SIZE;
                ptr += SECTOR_SIZE;
            }
            this->drives[driveId].size += SECTOR_SIZE;
            si++;
            if (8192 == si) {
                break; // size over
            }
        }
    }

    inline unsigned char read(unsigned char reg) {
        switch (reg) {
            case 4:
                if (~this->ctx.mainStatus & STM_RQM) {
                    this->ctx.mainStatus |= STM_RQM;
                }
                return (this->ctx.mainStatus & ~STM_NDM) | (this->ctx.phase == PHASE_DATATRANSFER ? STM_NDM : 0);
            case 5:
                switch (this->ctx.phase) {
                    case PHASE_DATATRANSFER:
                        reg = this->executionPhaseRead();
                        this->ctx.mainStatus &= ~STM_RQM;
                        return reg;
                    case PHASE_RESULT:
                        return this->resultsPhaseRead();
                }
            default:
                return 0x00;
        }
    }
    
    inline void write(unsigned char reg, unsigned char value) {
        switch (reg) {
            case 2:
                this->ctx.drive = value & 0x03;
                break;
            case 5:
                switch (this->ctx.phase) {
                    case PHASE_IDLE:
                        this->idlePhaseWrite(value);
                        break;
                    case PHASE_COMMAND:
                        this->commandPhaseWrite(value);
                        break;
                    case PHASE_DATATRANSFER:
                        this->executionPhaseWrite(value);
                        this->ctx.mainStatus &= ~STM_RQM;
                        break;
                }
                break;
        }
    }
    
private:
    inline bool diskPresent(int driveId) {
        return 0 <= driveId && driveId < NUMBER_OF_DRIVES && 0 < this->drives[driveId].size;
    }

    inline bool diskEnabled(int driveId) {
        return 0 <= driveId && driveId < NUMBER_OF_DRIVES;
    }

    inline int diskGetSides(int driveId) {
        return 0 <= driveId && driveId < NUMBER_OF_DRIVES ? NUMBER_OF_SIDES : 0;
    }

    inline bool diskReadOnly(int driveId) {
        return 0 <= driveId && driveId < NUMBER_OF_DRIVES ? this->drives[driveId].readOnly : false;
    }

    inline int diskReadSector(int driveId, int side, int track, int sector) {
        int offset = sector - 1 + NUMBER_OF_SECTORS * (track * NUMBER_OF_SIDES + side);
        if (8192 <= offset) {
            return 0;
        }
        if (this->CB.diskReadListener) {
            this->CB.diskReadListener(this->CB.arg, driveId, offset);
        }
        if (!this->diskPresent(driveId)) {
            return 0;
        } else if (side < 0 || NUMBER_OF_SIDES <= side) {
            return 0;
        } else if (track < 0 || NUMBER_OF_TRACKS <= track) {
            return 0;
        } else if (sector < 1 || NUMBER_OF_SECTORS < sector) {
            return 0;
        }
        memcpy(this->ctx.sectorBuf, this->drives[driveId].sectors[offset], SECTOR_SIZE);
        return 1;
    }

    inline int diskWriteSector(int driveId, int side, int track, int sector) {
        int offset = sector - 1 + NUMBER_OF_SECTORS * (track * NUMBER_OF_SIDES + side);
        if (8192 <= offset) {
            return 0;
        }
        if (this->CB.diskWriteListener) {
            this->CB.diskWriteListener(this->CB.arg, driveId, offset);
        }
        if (!this->diskPresent(driveId)) {
            return 0;
        } else if (this->diskReadOnly(driveId)) {
            return 0;
        } else if (side < 0 || NUMBER_OF_SIDES <= side) {
            return 0;
        } else if (track < 0 || NUMBER_OF_TRACKS <= track) {
            return 0;
        } else if (sector < 1 || NUMBER_OF_SECTORS < sector) {
            return 0;
        }
        memcpy(this->drives[driveId].sectors[offset], this->ctx.sectorBuf, SECTOR_SIZE);
        return 1;
    }

    inline void idlePhaseWrite(unsigned char value) {
        this->ctx.command = CMD_UNKNOWN;
        if ((value & 0x1f) == 0x06) this->ctx.command = CMD_READ_DATA;
        if ((value & 0x3f) == 0x05) this->ctx.command = CMD_WRITE_DATA;
        if ((value & 0x3f) == 0x09) this->ctx.command = CMD_WRITE_DELETED_DATA;
        if ((value & 0x1f) == 0x0c) this->ctx.command = CMD_READ_DELETED_DATA;
        if ((value & 0xbf) == 0x02) this->ctx.command = CMD_READ_DIAGNOSTIC;
        if ((value & 0xbf) == 0x0a) this->ctx.command = CMD_READ_ID;
        if ((value & 0xbf) == 0x0d) this->ctx.command = CMD_FORMAT;
        if ((value & 0x1f) == 0x11) this->ctx.command = CMD_SCAN_EQUAL;
        if ((value & 0x1f) == 0x19) this->ctx.command = CMD_SCAN_LOW_OR_EQUAL;
        if ((value & 0x1f) == 0x1d) this->ctx.command = CMD_SCAN_HIGH_OR_EQUAL;
        if ((value & 0xff) == 0x0f) this->ctx.command = CMD_SEEK;
        if ((value & 0xff) == 0x07) this->ctx.command = CMD_RECALIBRATE;
        if ((value & 0xff) == 0x08) this->ctx.command = CMD_SENSE_INTERRUPT_STATUS;
        if ((value & 0xff) == 0x03) this->ctx.command = CMD_SPECIFY;
        if ((value & 0xff) == 0x04) this->ctx.command = CMD_SENSE_DEVICE_STATUS;
        this->ctx.commandCode = value;
        this->ctx.phase = PHASE_COMMAND;
        this->ctx.phaseStep = 0;
        this->ctx.mainStatus |= STM_CB;
        switch (this->ctx.command) {
            case CMD_READ_DATA:
            case CMD_WRITE_DATA:
            case CMD_FORMAT:
                this->ctx.status[0] &= ~(ST0_IC0 | ST0_IC1);
                this->ctx.status[1] &= ~(ST1_ND | ST1_NW);
                this->ctx.status[2] &= ~ST2_DD;
                break;
            case CMD_RECALIBRATE:
                this->ctx.status[0] &= ~ST0_SE;
                break;
            case CMD_SENSE_INTERRUPT_STATUS:
                this->ctx.phase = PHASE_RESULT;
                this->ctx.mainStatus |= STM_DIO;
                break;
            case CMD_SEEK:
            case CMD_SPECIFY:
            case CMD_SENSE_DEVICE_STATUS:
                break;
            default:
                this->ctx.mainStatus &= ~STM_CB;
                this->ctx.phase = PHASE_IDLE;
        }
    }
    
    inline void commandPhaseWrite(unsigned char value) {
        switch (this->ctx.command) {
            case CMD_READ_DATA: this->commandSetupRW(value); break;
            case CMD_WRITE_DATA: this->commandSetupRW(value); break;
            case CMD_FORMAT: this->commandSetupFormat(value); break;
            case CMD_SEEK: this->commandSetupSeek(value); break;
            case CMD_RECALIBRATE: this->commandSetupRecalibrate(value); break;
            case CMD_SPECIFY: this->commandSetupSpecify(value); break;
            case CMD_SENSE_DEVICE_STATUS: this->commandSetupSenseDeviceStatus(value); break;
        }
    }
    
    inline void commandSetupRW(unsigned char value) {
        switch (this->ctx.phaseStep++) {
            case 0:
                this->ctx.status[0] &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
                this->ctx.status[0] |= this->diskPresent(this->ctx.drive) ? 0 : ST0_DS0;
                this->ctx.status[0] |= value & (ST0_DS0 | ST0_DS1);
                this->ctx.status[0] |= this->diskEnabled(this->ctx.drive) ? 0 : ST0_IC1;
                this->ctx.status[3] = value & (ST3_DS0 | ST3_DS1);
                this->ctx.status[3] |= this->ctx.currentTrack == 0 ? ST3_TK0 : 0;
                this->ctx.status[3] |= this->diskGetSides(this->ctx.drive) == 2 ? ST3_HD : 0;
                this->ctx.status[3] |= this->diskReadOnly(this->ctx.drive) ? ST3_WP : 0;
                this->ctx.status[3] |= this->diskPresent(this->ctx.drive) ? ST3_RDY : 0;
                break;
            case 1:
                this->ctx.cylinderNumber = value;
                break;
            case 2:
                this->ctx.side = value & 1;
                break;
            case 3:
                this->ctx.sectorNumber = value;
                break;
            case 4:
                this->ctx.number = value;
                this->ctx.sectorOffset = (value == 2 && (this->ctx.commandCode & 0xc0) == 0x40) ? 0 : 512;
                break;
            case 7:
                if (this->ctx.command == CMD_READ_DATA) {
                    int readCount = this->diskReadSector(this->ctx.drive, this->ctx.side, this->ctx.currentTrack, this->ctx.sectorNumber);
                    if (0 == readCount) {
                        this->ctx.status[0] |= ST0_IC0;
                        this->ctx.status[1] |= ST1_ND;
                    }
                    this->ctx.mainStatus |= STM_DIO;
                } else {
                    this->ctx.mainStatus &= ~STM_DIO;
                }
                this->ctx.phase = PHASE_DATATRANSFER;
                this->ctx.phaseStep = 0;
                break;
        }
    }
    
    inline void commandSetupFormat(unsigned char value) {
        switch (this->ctx.phaseStep++) {
            case 0:
                this->ctx.status[0] &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
                this->ctx.status[0] |= this->diskPresent(this->ctx.drive) ? 0 : ST0_DS0;
                this->ctx.status[0] |= value & (ST0_DS0 | ST0_DS1);
                this->ctx.status[0] |= this->diskEnabled(this->ctx.drive) ? 0 : ST0_IC1;
                this->ctx.status[3] = value & (ST3_DS0 | ST3_DS1);
                this->ctx.status[3] |= this->ctx.currentTrack == 0 ? ST3_TK0 : 0;
                this->ctx.status[3] |= this->diskGetSides(this->ctx.drive) == 2 ? ST3_HD : 0;
                this->ctx.status[3] |= this->diskReadOnly(this->ctx.drive) ? ST3_WP : 0;
                this->ctx.status[3] |= this->diskPresent(this->ctx.drive) ? ST3_RDY : 0;
                break;
            case 1:
                this->ctx.number = value;
                break;
            case 2:
                this->ctx.sectorsPerCylinder = value;
                this->ctx.sectorNumber = value;
                break;
            case 4:
                this->ctx.fillerByte = value;
                this->ctx.sectorOffset = 0;
                this->ctx.mainStatus &= ~STM_DIO;
                this->ctx.phase = PHASE_DATATRANSFER;
                this->ctx.phaseStep = 0;
                break;
        }
    }
    
    inline void commandSetupSeek(unsigned char value) {
        switch (this->ctx.phaseStep++) {
            case 0:
                this->ctx.status[0] &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
                this->ctx.status[0] |= this->diskPresent(this->ctx.drive) ? 0 : ST0_DS0;
                this->ctx.status[0] |= value & (ST0_DS0 | ST0_DS1);
                this->ctx.status[0] |= this->diskEnabled(this->ctx.drive) ? 0 : ST0_IC1;
                this->ctx.status[3] = value & (ST3_DS0 | ST3_DS1);
                this->ctx.status[3] |= this->ctx.currentTrack == 0 ? ST3_TK0 : 0;
                this->ctx.status[3] |= this->diskGetSides(this->ctx.drive) == 2 ? ST3_HD : 0;
                this->ctx.status[3] |= this->diskReadOnly(this->ctx.drive) ? ST3_WP : 0;
                this->ctx.status[3] |= this->diskPresent(this->ctx.drive) ? ST3_RDY : 0;
                break;
            case 1:
                this->ctx.currentTrack = value;
                this->ctx.status[0] |= ST0_SE;
                this->ctx.mainStatus &= ~STM_CB;
                this->ctx.phase = PHASE_IDLE;
                break;
        }
    }
    
    inline void commandSetupRecalibrate(unsigned char value) {
        switch (this->ctx.phaseStep++) {
            case 0:
                this->ctx.status[0] &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
                this->ctx.status[0] |= this->diskPresent(this->ctx.drive) ? 0 : ST0_DS0;
                this->ctx.status[0] |= value & (ST0_DS0 | ST0_DS1);
                this->ctx.status[0] |= this->diskEnabled(this->ctx.drive) ? 0 : ST0_IC1;
                this->ctx.status[0] |= ST0_SE;
                this->ctx.status[3] = value & (ST3_DS0 | ST3_DS1);
                this->ctx.status[3] |= this->ctx.currentTrack == 0 ? ST3_TK0 : 0;
                this->ctx.status[3] |= this->diskGetSides(this->ctx.drive) == 2 ? ST3_HD : 0;
                this->ctx.status[3] |= this->diskReadOnly(this->ctx.drive) ? ST3_WP : 0;
                this->ctx.status[3] |= this->diskPresent(this->ctx.drive) ? ST3_RDY : 0;
                this->ctx.currentTrack = 0;
                this->ctx.mainStatus &= ~STM_CB;
                this->ctx.phase = PHASE_IDLE;
                break;
        }
    }
    
    inline void commandSetupSpecify(unsigned char value) {
        switch (this->ctx.phaseStep++) {
            case 1:
                this->ctx.mainStatus &= ~STM_CB;
                this->ctx.phase = PHASE_IDLE;
                break;
        }
    }
    
    inline void commandSetupSenseDeviceStatus(unsigned char value) {
        switch (this->ctx.phaseStep++) {
            case 0:
                this->ctx.status[0] &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
                this->ctx.status[0] |= this->diskPresent(this->ctx.drive) ? 0 : ST0_DS0;
                this->ctx.status[0] |= value & (ST0_DS0 | ST0_DS1);
                this->ctx.status[0] |= this->diskEnabled(this->ctx.drive) ? 0 : ST0_IC1;
                this->ctx.status[3] = value & (ST3_DS0 | ST3_DS1);
                this->ctx.status[3] |= this->ctx.currentTrack == 0 ? ST3_TK0 : 0;
                this->ctx.status[3] |= this->diskGetSides(this->ctx.drive) == 2 ? ST3_HD : 0;
                this->ctx.status[3] |= this->diskReadOnly(this->ctx.drive) ? ST3_WP : 0;
                this->ctx.status[3] |= this->diskPresent(this->ctx.drive) ? ST3_RDY : 0;
                this->ctx.phase = PHASE_RESULT;
                this->ctx.phaseStep = 0;
                this->ctx.mainStatus |= STM_DIO;
                break;
        }
    }
    
    inline void executionPhaseWrite(unsigned char value) {
        switch (this->ctx.command) {
            case CMD_WRITE_DATA: this->executionWriteData(value); break;
            case CMD_FORMAT: this->executionFormat(value); break;
        }
    }
    
    inline void executionWriteData(unsigned char value) {
        if (this->ctx.sectorOffset < SECTOR_SIZE) {
            this->ctx.sectorBuf[this->ctx.sectorOffset++] = value;
            if (this->ctx.sectorOffset == SECTOR_SIZE) {
                int ret = this->diskWriteSector(this->ctx.drive,
                                                this->ctx.side,
                                                this->ctx.currentTrack,
                                                this->ctx.sectorNumber);
                this->ctx.status[1] |= !ret ? ST1_NW : 0;
                this->ctx.phase = PHASE_RESULT;
                this->ctx.phaseStep = 0;
                this->ctx.mainStatus |= STM_DIO;
            }
        }
    }
    
    inline void executionFormat(unsigned char value) {
        switch(this->ctx.phaseStep & 3) {
            case 0:
                this->ctx.currentTrack = value;
                break;
            case 1:
                memset(this->ctx.sectorBuf, this->ctx.fillerByte, 512);
                //int ret = this->diskWrite(this->ctx.drive, this->ctx.sectorBuf, this->ctx.sectorNumber - 1 + this->diskGetSectorsPerTrack(this->ctx.drive) * (this->ctx.currentTrack * this->diskGetSides(this->ctx.drive) + value));
                //this->ctx.status[1] |= !ret ? ST1_NW : 0;
                this->ctx.status[1] |= ST1_NW;
                break;
            case 2:
                this->ctx.sectorNumber = value;
                break;
        }
        if (++this->ctx.phaseStep == 4 * this->ctx.sectorsPerCylinder - 2) {
            this->ctx.phase = PHASE_RESULT;
            this->ctx.phaseStep = 0;
            this->ctx.mainStatus |= STM_DIO;
        }
    }
    
    inline unsigned char executionPhaseRead(){
        if (this->ctx.command == CMD_READ_DATA) {
            if (this->ctx.sectorOffset < 512) {
                unsigned char value = this->ctx.sectorBuf[this->ctx.sectorOffset++];
                if (this->ctx.sectorOffset == 512) {
                    this->ctx.phase = PHASE_RESULT;
                    this->ctx.phaseStep = 0;
                }
                return value;
            }
        }
        return 0xff;
    }
    
    inline unsigned char resultsPhaseRead() {
        switch (this->ctx.command) {
            case CMD_READ_DATA:
            case CMD_WRITE_DATA:
            case CMD_FORMAT:
                switch (this->ctx.phaseStep++) {
                    case 0: return this->ctx.status[0];
                    case 1: return this->ctx.status[1];
                    case 2: return this->ctx.status[2];
                    case 3: return this->ctx.cylinderNumber;
                    case 4: return this->ctx.side;
                    case 5: return this->ctx.sectorNumber;
                    case 6:
                        this->ctx.phase = PHASE_IDLE;
                        this->ctx.mainStatus &= ~STM_CB;
                        this->ctx.mainStatus &= ~STM_DIO;
                        return this->ctx.number;
                }
                break;
            case CMD_SENSE_INTERRUPT_STATUS:
                switch (this->ctx.phaseStep++) {
                    case 0:
                        return this->ctx.status[0];
                    case 1:
                        this->ctx.phase = PHASE_IDLE;
                        this->ctx.mainStatus &= ~(STM_CB | STM_DIO);
                        return this->ctx.currentTrack;
                }
                break;
            case CMD_SENSE_DEVICE_STATUS:
                switch (this->ctx.phaseStep++) {
                    case 0:
                        this->ctx.phase = PHASE_IDLE;
                        this->ctx.mainStatus &= ~(STM_CB | STM_DIO);
                        return this->ctx.status[3];
                }
                break;
        }
        return 0xff;
    }
};

#endif // INCLUDE_TC8566AF_HPP
