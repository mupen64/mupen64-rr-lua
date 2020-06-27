#include "disasm.h"	//main\disasm.h,main\disasm.c‚ª•K—v
#include <stdio.h>
#include <time.h>
/* ‚â‚Á‚Â‚¯ */

FILE *inf, *outf;

void TraceLogging(r4300word pc, r4300word w, r4300word r0, r4300word r1, int last) {
	static char buf[0x1000000];
	static char * p = buf;
	const char * const buflength = buf + sizeof(buf) - 512;
	INSTDECODE decode;
	const char * const x = "0123456789abcdef";
	#define HEX8(n) p[0] = x[(r4300word)(n)>>28&0xF];\
		p[1] = x[(r4300word)(n)>>24&0xF];\
		p[2] = x[(r4300word)(n)>>20&0xF];\
		p[3] = x[(r4300word)(n)>>16&0xF];\
		p[4] = x[(r4300word)(n)>>12&0xF];\
		p[5] = x[(r4300word)(n)>>8&0xF];\
		p[6] = x[(r4300word)(n)>>4&0xF];\
		p[7] = x[(r4300word)(n)&0xF];\
		p+=8;

	if(!last) {

		DecodeInstruction(w, &decode);
		HEX8(pc);
		*(p++) = ':';
		*(p++) = ' ';
		HEX8(w);
		*(p++) = ' ';
		const char *ps = p;
		if(w == 0x00000000) {
			*(p++) = 'n';
			*(p++) = 'o';
			*(p++) = 'p';
		} else {
			for(const char *q = GetOpcodeString(&decode); *q; q ++) {
				*(p++) = *q;
			}
			*(p++) = ' ';
			p = GetOperandString(p, &decode, pc);
		}
		for(int i = p-ps+3; i < 24; i += 4) {
			*(p++) = '\t';
		}
		*(p++) = ';';
		INSTOPERAND &o = decode.operand;
		#define REGCPU(n, b) if((n)!=0) {\
					for(const char *l = CPURegisterName[n]; *l; l++) {\
						*(p++) = *l;\
					}\
					*(p++) = '=';\
					HEX8(r##b);\
			}
		#define REGCPU2(n,m) \
				REGCPU(n,0);\
				if((n)!=(m)&&(m)!=0) {C;REGCPU(m,1);
			}
		//10i”
		#define REGFPU(n, b) *(p++)='f';\
					*(p++)=x[(n)/10];\
					*(p++)=x[(n)%10];\
					*(p++) = '=';\
					p+=sprintf(p,"%f",*(float*)&r##b)
		#define REGFPU2(n,m) REGFPU(n,0);\
				if((n)!=(m)) {C;REGFPU(m,1);
			}
		#define C *(p++) = ','

			/*
			if(delay_slot) {
				*(p++) = '#';
			}
			*/

		switch(decode.format) {
			case INSTF_NONE:
				break;
			case INSTF_J:
			case INSTF_0BRANCH:
				break;
			case INSTF_LUI:
				break;
			case INSTF_1BRANCH:
			case INSTF_JR:
			case INSTF_ISIGN:
			case INSTF_IUNSIGN:
				REGCPU(o.i.rs,0);
				break;
			case INSTF_2BRANCH:
				REGCPU2(o.i.rs,o.i.rt);
				break;
			case INSTF_ADDRW:
				REGCPU(o.i.rt,1);
				if(o.i.rt!=0) {C;
			}
			case INSTF_ADDRR:
				*(p++) = '@';
				*(p++) = '=';
				HEX8(r0);
				break;
			case INSTF_LFW:
				REGFPU(o.lf.ft,1);
			case INSTF_LFR:
				*(p++) = '@';
				*(p++) = '=';
				HEX8(r0);
				break;
			case INSTF_R1:
				REGCPU(o.r.rd,0);
				break;
			case INSTF_R2:
				REGCPU2(o.i.rs,o.i.rt);
				break;
			case INSTF_R3:
				REGCPU2(o.i.rs,o.i.rt);
				break;
			case INSTF_MTC0:
			case INSTF_MTC1:
			case INSTF_SA:
				REGCPU(o.r.rt,0);
				break;
			case INSTF_R2F:
				REGFPU(o.cf.fs,0);
				break;
			case INSTF_R3F:
			case INSTF_C:
				REGFPU2(o.cf.fs,o.cf.ft);
				break;
			case INSTF_MFC0:
				break;
			case INSTF_MFC1:
				REGFPU((FPUREG)o.r.rs,0);
				break;
		}
		*(p++) = '\n';
	}
	if(last || p >= buflength) {
		fwrite(buf, 1, p-buf, outf);
		p = buf;
	}
#undef HEX8
#undef REGCPU
#undef REGFPU
#undef REGCPU2
#undef REGFPU2
#undef C

}

typedef unsigned long long filesize_t;
int main(int argc, char *argv[]) {
	static r4300word buf[0x100000];
	size_t readsize;
	filesize_t in_allsize, in_cntsize, out_cntsize;
	unsigned long long out_cntlines = 0;
	clock_t clk;

	clk = clock();
	if(argc<=1) {
		puts("argument count <= 1!\nbintracefmt input_file [output_file('bintrace_fmt.log')]");
		while(getchar()!='\n');
		return 1;
	}
	inf = fopen(argv[1], "rb");
	outf = fopen(argc>=3?argv[2]:"bintrace_fmt.log", "wb");
	_fseeki64(inf, 0, SEEK_END);
	in_allsize = _ftelli64(inf);
	if(in_allsize%0x10) {
		printf("inputfile is bad? (size%0x10!=0)");
	}
	in_allsize = in_allsize/4*4;
	_fseeki64(inf, 0, SEEK_SET);
	in_cntsize = 0;
	do {
		size_t i;
		readsize = fread(buf, 4, sizeof(buf)/4, inf);
		for(i = 0; i < readsize/4; i ++) {
			TraceLogging(buf[i*4+0], buf[i*4+1], buf[i*4+2], buf[i*4+3], 0);
		}
		out_cntlines += readsize/4;
		in_cntsize += readsize*4;
		out_cntsize = _ftelli64(outf);
		if(!readsize) {
			TraceLogging(0, 0, 0, 0, 1);
		}
		printf("\rin: %.2fMiB / %.2fMiB (%.3f%%)\tout: %llulines %.2fMiB",
			in_cntsize/1024.0/1024, in_allsize/1024.0/1024,
			(double)in_cntsize/in_allsize*100,
			out_cntlines,
			out_cntsize/1024.0/1024);
	} while(readsize);
	//flush
	fclose(inf);
	fclose(outf);
	printf("\ncompleted! %.3fsec.\n", (clock()-clk)/1000.0);
	return 0;
}
