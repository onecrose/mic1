#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// types

typedef unsigned char byte; // 8 bits
typedef unsigned int word; // 32 bits
typedef unsigned long int Microinstruction; // 64 bits, in this case according to the architecture each microinstruction uses only 36bits of space.

// Registers 

word MAR = 0, MDR = 0, PC = 0; // Memory acess
byte MBR = 0;			  // Memory acess

word SP = 0, LV = 0, TOS = 0, // ULA Operation
		OPC = 0, CPP = 0, H = 0; // ULA Operation

Microinstruction MIR; // Contains the Current Microinstruction
word MPC = 0; // Contains the address for the next Microinstruction

// Bus

word Bus_B, Bus_C;

// Flip-Flops

byte N, Z;

// Microinstruction Decoding Aids

byte MIR_B, MIR_Operation, MIR_Displacer, MIR_MEM, MIR_jump;
word MIR_C;


// Control Storage

Microinstruction Storage[512];

// Main Memory

byte Memory[100000000];

// Function prototype

void load_control_microprogram();
void load_program(const char *program);
void show_processes();
void decode_Microinstruction();
void assign_bus_B();
void perform_operation_ALU();
void assign_bus_C();
void operate_memory();
void jump();

void binary(void* value, int type);



// Main Loop

int main(int argc, const char *argv[]){
	load_control_microprogram();
	load_program(argv[1]);
	while(1){
		show_processes();
		MIR = Storage[MPC];

		
		decode_Microinstruction();
		assign_bus_B();
		perform_operation_ALU();
		assign_bus_C();
		operate_memory();
		jump();
	}

	return 0;
}

// Implementation of functions

void load_control_microprogram(){
	FILE* MicroProgram;
	MicroProgram = fopen("microprog.rom", "rb");

	if(MicroProgram != NULL){
		fread(Storage, sizeof(Microinstruction), 512, MicroProgram);
		
		fclose(MicroProgram);
	}

}

void load_program(const char* prog){
	FILE* Program;
	word size;
	byte size_temp[4];
	Program = fopen(prog, "rb");

	if(Program != NULL){
		fread(size_temp, sizeof(byte), 4, Program); // Reading the size in bytes of the Program
		memcpy(&size, size_temp, 4);

		fread(Memory, sizeof(byte), 20, Program); // Reading the 20 bytes of Initialization
		fread(&Memory[0x0401], sizeof(byte), size - 20, Program); // Reading Program

	}
}

void decode_Microinstruction(){
	MIR_B = (MIR) & 0b1111;
	MIR_MEM = (MIR >> 4) & 0b111;
	MIR_C = (MIR >> 7) & 0b111111111;
	MIR_Operation = (MIR >> 16) & 0b111111;
	MIR_Displacer = (MIR >> 22) & 0b11;
	MIR_jump = (MIR >> 24) & 0b111;
	MPC = (MIR >> 27) & 0b111111111;
}

void assign_bus_B(){
	switch(MIR_B){
		case 0: Bus_B = MDR;				break;
		case 1: Bus_B = PC;				break;
		//Case 2 loads the MBR with signal by doing the signal extension, i.e. copying the most significant bit of the MBR to the 24 most significant positions of the B bus.
		case 2: Bus_B = MBR;
			if(MBR & (0b10000000))
					Bus_B = Bus_B | (0b111111111111111111111111 << 8);
										break;
		case 3: Bus_B = MBR;				break;
		case 4: Bus_B = SP;				break;
		case 5: Bus_B = LV;				break;
		case 6: Bus_B = CPP;				break;
		case 7: Bus_B = TOS;				break;
		case 8: Bus_B = OPC;				break;
		default: Bus_B = -1;				break; 
	}
}

void realizar_operacao_ALU(){
	switch(MIR_Operacao){
		//Cada operação da ULA é representado pela sequencia dos bits de operação. Cada operação útil foi convertida para inteiro para facilitar a escrita do switch
		case 12: Bus_C = H & Bus_B;		break;
		case 17: Bus_C = 1;			break;
		case 18: Bus_C = -1;			break;
		case 20: Bus_C = Bus_B;			break;
		case 24: Bus_C = H;			break;
		case 26: Bus_C = ~H;			break;
		case 28: Bus_C = H | Bus_B;		break;
		case 44: Bus_C = ~Bus_B;			break;
		case 53: Bus_C = Bus_B + 1;		break;
		case 54: Bus_C = Bus_B - 1;		break;
		case 57: Bus_C = H + 1;		break;
		case 59: Bus_C = -H;			break;
		case 60: Bus_C = H + Bus_B;		break;
		case 61: Bus_C = H + Bus_B + 1;	break;
		case 63: Bus_C = Bus_B - H;		break;

		default: break;
	}

	if(Bus_C){
		N = 0;
		Z = 1;
	} else{
		N = 1;
		Z = 0;
	}

	switch(MIR_Displacer){
		case 1: Bus_C = Bus_C << 8; break;
		case 2: Bus_C = Bus_C >> 1; break;		
	}
}

void assign_Bus_C(){
	if(MIR_C & 0b000000001)   MAR = Bus_C;
	if(MIR_C & 0b000000010)   MDR = Bus_C;
	if(MIR_C & 0b000000100)   PC  = Bus_C;
	if(MIR_C & 0b000001000)   SP  = Bus_C;
	if(MIR_C & 0b000010000)   LV  = Bus_C;
	if(MIR_C & 0b000100000)   CPP = Bus_C;
	if(MIR_C & 0b001000000)   TOS = Bus_C;
	if(MIR_C & 0b010000000)   OPC = Bus_C;
	if(MIR_C & 0b100000000)   H   = Bus_C;
}

void operate_memory(){
	// Multiplication by 4 is necessary because the MAR and MDR ports read consecutive words in Memory
	if(MIR_MEM & 0b001) MBR = Memory[PC]; 
	if(MIR_MEM & 0b010) memcpy(&MDR, &Memory[MAR*4], 4);
	if(MIR_MEM & 0b100) memcpy(&Memory[MAR*4], &MDR, 4);
}

void jump(){
	if(MIR_jump & 0b001) MPC = MPC | (N << 8);
	if(MIR_jump & 0b010) MPC = MPC | (Z << 8);
	if(MIR_jump & 0b100) MPC = MPC | (MBR);
}

void show_processes(){

	if(LV && SP){
		printf("\t\t  STACK OF OPERANDS\n");
		printf("========================================\n");
		printf("     END");
		printf("\t   VALUE BINARY");
		printf(" \t\tVALUE\n");
		for(int i = SP ; i >= LV ; i--){
			word value;
			memcpy(&value, &Memory[i*4],4);			
			
			if(i == SP) printf("SP ->");
			else if(i == LV) printf("LV ->");
			else printf("     ");

			printf("%X ",i);
			binary(&value, 1); printf(" ");
			printf("%d\n", value);
		}

		printf("========================================\n");
	}

	if(PC >= 0x0401) {
		printf("\n\t\t\tProgram Area\n");
		printf("========================================\n");
		printf("\t\tbinary");
		printf("\t HEX");
		printf("  BYTE END\n");
		for(int i = PC - 2; i <= PC + 3 ; i++){
			if(i == PC) printf("In Progress >>  ");
			else printf("\t\t");
			binary(&Memory[i], 2);
			printf(" 0x%02X ", Memory[i]);
			printf("\t%X\n", i);
		}
		printf("========================================\n\n");
	}

	printf("\t\tREGISTERS\n");
	printf("\tbinary");
	printf("\t\t\t\tHEX\n");
	printf("MAR: "); binary(&MAR, 3); printf("\t%x\n", MAR);
	printf("MDR: "); binary(&MDR, 3); printf("\t%x\n", MDR);
	printf("PC:  "); binary(&PC, 3); printf("\t%x\n", PC);
	printf("MBR: \t\t"); binary(&MBR, 2); printf("\t\t%x\n", MBR);
	printf("SP:  "); binary(&SP,3); printf("\t%x\n", SP);
	printf("LV:  "); binary(&LV,3); printf("\t%x\n", LV);
	printf("CPP: "); binary(&CPP,3); printf("\t%x\n", CPP);
	printf("TOS: "); binary(&TOS,3); printf("\t%x\n", TOS);
	printf("OPC: "); binary(&OPC, 3); printf("\t%x\n", OPC);
	printf("H:   "); binary(&H, 3); printf("\t%x\n", H);

	printf("MPC: \t\t"); binary(&MPC, 5); printf("\t%x\n", MPC);
	printf("MIR: "); binary(&MIR, 4); printf("\n");

	getchar();
}


//FUNCTION RESPONSIBLE FOR PRINTING THE BINARY VALUES
//TYPE 1: Prints the binary of 4 bytes in a row 
//TYPE 2: Prints the binary of 1 byte
//TYPE 3: Prints the binary of a word 
//TYPE 4: Prints the binary of a microinstruction
//TYPE 5: Prints the binary of the 9 bits of the MPC
void binary(void* value, int type){
	switch(type){
		case 1: {
			printf(" ");
			byte aux;
			byte* valuAux = (byte*)value;
				
			for(int i = 3; i >= 0; i--){
				aux = *(valueAux + i);
				for(int j = 0; j < 8; j++){
					printf("%d", (aux >> 7) & 0b1);
					aux = aux << 1;
				}
				printf(" ");
			}		
		} break;
		case 2:{
			byte aux;
			
			aux = *((byte*)(value));
			for(int j = 0; j < 8; j++){
				printf("%d", (aux >> 7) & 0b1);
				aux = aux << 1;
			}
		} break;

		case 3: {
			word aux;			
			aux = *((word*)(value));
			for(int j = 0; j < 32; j++){
				printf("%d", (aux >> 31) & 0b1);
				aux = aux << 1;
			}
		} break;

		case 4: {
			Microinstruction aux;		
			aux = *((Microinstruction*)(value));
			for(int j = 0; j < 36; j++){
				if ( j == 9 || j == 12 || j == 20 || j == 29 || j == 32) printf(" ");

				printf("%ld", (aux >> 35) & 0b1);
				aux = aux << 1;
			}
		} break;
		case 5: {
			word aux;
		
			aux = *((word*)(value)) << 23;
			for(int j = 0; j < 9; j++){
				printf("%d", (aux >> 31) & 0b1);
				aux = aux << 1;
			}
		} break;
	}
}
