// Name: Naman Sharma
// Professor: Ms Bashira Akter Anima
// Project 3: Conditional Execution, Status Flags and Control Flow in an Assembly Simulator

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

// Global register array: R0 to R11 (12 registers total)
// All initialized to 0

unsigned int reg[12] = {0};

// Memory array: 5 locations from 0x100 to 0x110
// Index 0 = 0x100, Index 1 = 0x104, Index 2 = 0x108,
// Index 3 = 0x10C, Index 4 = 0x110
// All initialized to 0

unsigned int mem[5] = {0};

// -------------------------------------------------------
// Track which memory slots have been written to
// so we know whether to print "___" or the hex value
// -------------------------------------------------------
bool memWritten[5] = {false, false, false, false, false};

// -------------------------------------------------------
// NZCV flags:
// N = Negative flag
// Z = Zero flag
// C = Carry flag
// V = Overflow flag
// -------------------------------------------------------
int flagN = 0;
int flagZ = 0;
int flagC = 0;
int flagV = 0;


// -------------------------------------------------------
// Convert a memory address
// an index in our 5-element memory array.
// 0x100 = index 0, 0x104 = index 1, etc.
// -------------------------------------------------------
int addressToIndex(unsigned int address)
{
    return (int)((address - 0x100) / 4);
}


// -------------------------------------------------------
// Print the current state of all 12 registers
// in hexadecimal format
// -------------------------------------------------------
void printRegisters()
{
    cout << " Register array: " <<endl;
    cout << " R0 =0x" << hex << reg[0]
              << " R1=0x"  << reg[1]
              << " R2=0x"  << reg[2]
              << " R3=0x"  << reg[3]
              << " R4=0x"  << reg[4]
              << " R5=0x"  << reg[5]
              <<endl;
    cout << " R6=0x"  << reg[6]
              << " R7=0x"  << reg[7]
              << " R8=0x"  << reg[8]
              << " R9=0x"  << reg[9]
              << " R10=0x" << reg[10]
              << " R11=0x" << reg[11]
              <<endl;
    cout <<dec;
}


// -------------------------------------------------------
// Print the NZCV flags as 4 binary digits (0 or 1)
// -------------------------------------------------------
void printFlags()
{
    cout << " NZCV: "
              << flagN << flagZ << flagC << flagV
              <<endl;
}


// -------------------------------------------------------
// Print the memory array.
// Slots that have not been written show as "___"
// Slots that have been written show their hex value
// -------------------------------------------------------
void printMemory()
{
    cout << " Memory array:" << endl;
    cout << " ";
    for (int i = 0; i < 5; i++)
    {
        if (memWritten[i])
        {
            cout << "0x" << hex << mem[i];
        }
        else
        {
            cout << "___";
        }

        if (i < 4)
        {
            cout << ",";
        }
    }
    cout << endl;
    cout << dec;
}


// -------------------------------------------------------
// Print one instruction line followed by register state,
// flags, and memory. Called after every instruction.
// -------------------------------------------------------
void printAll(const string& instrLine)
{
    cout << " " << instrLine << endl;
    printRegisters();
    printFlags();
    printMemory();
    cout << endl;
}


// -------------------------------------------------------
// Parse a register token like "R0", "R11"
// (may have trailing commas which are ignored)
// Returns the register index 0-11
// -------------------------------------------------------
int parseRegIndex(const string& token)
{
    string clean = "";
    for (int i = 0; i < (int)token.size(); i++)
    {
        char c = token[i];
        if (c != ',' && c != '[' && c != ']' && c != ' ')
        {
            clean += c;
        }
    }
    // Remove the leading 'R' or 'r'
    if (!clean.empty() && (clean[0] == 'R' || clean[0] == 'r'))
    {
        clean = clean.substr(1);
    }
    return stoi(clean);
}


// -------------------------------------------------------
// Parse an immediate value token like "#10", "#0x14"
// Removes '#' and parses decimal or hex
// Returns the uint32_t value
// -------------------------------------------------------
unsigned int parseImmediate(const string& token)
{
    string clean = "";
    for (int i = 0; i < (int)token.size(); i++)
    {
        char c = token[i];
        if (c != '#' && c != ',' && c != ' ')
        {
            clean += c;
        }
    }

    // Check if it is a hex value (starts with 0x or 0X)
    if (clean.size() >= 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X'))
    {
        return (unsigned int)stoul(clean, nullptr, 16);
    }
    else
    {
        return (unsigned int)stoul(clean, nullptr, 10);
    }
}

// Update NZCV flags after an arithmetic operation
// (ADD, SUB, CMP).
// result64  = 64-bit result to detect carry
// op1, op2  = original 32-bit operands before the op
// isSub     = true if this was a subtraction

void updateFlagsArithmetic(unsigned long long result64,
                           unsigned int op1,
                           unsigned int op2,
                           bool isSub)
{
    unsigned int result32 = (unsigned int)(result64 & 0xFFFFFFFF);

    // N flag: set if bit 31 is 1 (result looks negative in signed view)
    flagN = (int)((result32 >> 31) & 1);

    // Z flag: set if result is exactly zero
    flagZ = (result32 == 0) ? 1 : 0;

    if (isSub)
    {
        // C flag for subtraction: set if there was NO borrow (op1 >= op2)
        flagC = (op1 >= op2) ? 1 : 0;

        // V flag: signed overflow on subtraction
        // Overflow when: positive - negative = negative, or negative - positive = positive
        int s1 = (int)op1;
        int s2 = (int)op2;
        int sr = (int)result32;
        bool overflowed = ((s1 >= 0 && s2 < 0 && sr < 0) ||
                           (s1 < 0  && s2 >= 0 && sr >= 0));
        flagV = overflowed ? 1 : 0;
    }
    else
    {
        // C flag for addition: set if result did not fit in 32 bits
        flagC = (result64 > 0xFFFFFFFF) ? 1 : 0;

        // V flag: signed overflow on addition
        // Overflow when: positive + positive = negative, or negative + negative = positive
        int s1 = (int)op1;
        int s2 = (int)op2;
        int sr = (int)result32;
        bool overflowed = ((s1 > 0 && s2 > 0 && sr < 0) ||
                           (s1 < 0 && s2 < 0 && sr >= 0));
        flagV = overflowed ? 1 : 0;
    }
}


// -------------------------------------------------------
// Update only N and Z flags after a logical/bitwise op
// (AND, ORR, EOR, LSL, LSR, MOV, MVN with S suffix).
// C and V flags are NOT changed by logical operations.
// -------------------------------------------------------
void updateFlagsLogical(unsigned int result)
{
    flagN = (int)((result >> 31) & 1);
    flagZ = (result == 0) ? 1 : 0;
    // C and V remain unchanged
}


// -------------------------------------------------------
// Check whether a condition suffix is met given current flags.
// Returns true if the condition is satisfied.
// An empty condition string means unconditional (always true).
// -------------------------------------------------------
bool checkCondition(const string& cond)
{
    if (cond == "GT")
    {
        // Greater Than: Z is 0 AND N equals V
        return (flagZ == 0 && flagN == flagV);
    }
    if (cond == "GE")
    {
        // Greater or Equal: N equals V
        return (flagN == flagV);
    }
    if (cond == "LT")
    {
        // Less Than: N does not equal V
        return (flagN != flagV);
    }
    if (cond == "LE")
    {
        // Less or Equal: Z is 1 OR N does not equal V
        return (flagZ == 1 || flagN != flagV);
    }
    if (cond == "EQ")
    {
        // Equal: Z is 1
        return (flagZ == 1);
    }
    if (cond == "NE")
    {
        // Not Equal: Z is 0
        return (flagZ == 0);
    }
    // No condition suffix means always execute
    return true;
}


// -------------------------------------------------------
// Parse an opcode string and extract:
//   baseOp    = the core operation (ADD, SUB, MOV, etc.)
//   condition = condition suffix if any (GT, LT, EQ, etc.)
//   hasS      = true if the 'S' flag update suffix is present
//
// Examples:
//   "ADDGT"  -> baseOp="ADD", condition="GT", hasS=false
//   "ADDS"   -> baseOp="ADD", condition="",   hasS=true
//   "ORRS"   -> baseOp="ORR", condition="",   hasS=true
//   "LSRGE"  -> baseOp="LSR", condition="GE", hasS=false
//   "MVNEQ"  -> baseOp="MVN", condition="EQ", hasS=false
// -------------------------------------------------------
void parseOpcode(const string& rawOpcode,
                 string& baseOp,
                 string& condition,
                 bool& hasS)
{
    hasS = false;
    condition = "";
    baseOp = rawOpcode;

    // Convert to uppercase for consistent matching
    string upper = rawOpcode;
    for (int i = 0; i < (int)upper.size(); i++)
    {
        upper[i] = (char)toupper(upper[i]);
    }

    // All base opcodes we support (3-letter ones)
    vector<string> bases = {
        "ADD", "SUB", "AND", "ORR", "EOR",
        "LSL", "LSR", "MOV", "MVN",
        "LDR", "STR", "CMP", "BEQ"
    };

    // All valid condition suffixes
    vector<string> conditions = {
        "GT", "GE", "LT", "LE", "EQ", "NE"
    };

    for (int b = 0; b < (int)bases.size(); b++)
    {
        string base = bases[b];
        int baseLen = (int)base.size();

        // Check if the opcode starts with this base
        if ((int)upper.size() >= baseLen && upper.substr(0, baseLen) == base)
        {
            string rest = upper.substr(baseLen);

            // Nothing after base: simple unconditional, no S
            if (rest == "")
            {
                baseOp = base;
                return;
            }

            // Just "S" after base: flag update, no condition
            if (rest == "S")
            {
                baseOp = base;
                hasS = true;
                return;
            }

            // Check for condition suffix (with optional trailing S)
            for (int c = 0; c < (int)conditions.size(); c++)
            {
                string cond = conditions[c];

                if (rest == cond)
                {
                    baseOp = base;
                    condition = cond;
                    return;
                }

                if (rest == cond + "S")
                {
                    baseOp = base;
                    condition = cond;
                    hasS = true;
                    return;
                }
            }
        }
    }

    // If nothing matched, return the raw opcode uppercase as-is
    baseOp = upper;
}


// -------------------------------------------------------
// Remove all commas from a string token
// -------------------------------------------------------
string stripComma(const string& s)
{
    string result = "";
    for (int i = 0; i < (int)s.size(); i++)
    {
        if (s[i] != ',') result += s[i];
    }
    return result;
}


// -------------------------------------------------------
// Remove square brackets [ and ] from a string token
// Used for parsing memory operands like [R6]
// -------------------------------------------------------
string stripBrackets(const string& s)
{
    string result = "";
    for (int i = 0; i < (int)s.size(); i++)
    {
        if (s[i] != '[' && s[i] != ']') result += s[i];
    }
    return result;
}


// -------------------------------------------------------
// Check if a string (uppercase) starts with a known opcode
// Used to distinguish labels from instructions
// -------------------------------------------------------
bool isOpcodeStart(const string& upperToken)
{
    vector<string> opStarts = {
        "ADD", "SUB", "AND", "ORR", "EOR",
        "LSL", "LSR", "MOV", "MVN",
        "LDR", "STR", "CMP", "BEQ", "B"
    };
    for (int i = 0; i < (int)opStarts.size(); i++)
    {
        string op = opStarts[i];
        if ((int)upperToken.size() >= (int)op.size() &&
            upperToken.substr(0, op.size()) == op)
        {
            return true;
        }
    }
    return false;
}


// -------------------------------------------------------
// Execute one instruction given its tokens and print state.
// tokens     = all whitespace-split tokens of the line
// instrStart = index in tokens where the opcode lives
//              (1 if there's a label prefix, else 0)
// rawLine    = original full text of the line for display
// doPrint    = whether to print register/flag/mem state after
// -------------------------------------------------------
void executeInstruction(const vector<string>& tokens,
                        int instrStart,
                        const string& rawLine,
                        bool doPrint)
{
    if (instrStart >= (int)tokens.size()) return;

    string opcodeRaw = tokens[instrStart];
    string upperOpcode = opcodeRaw;
    for (int i = 0; i < (int)upperOpcode.size(); i++)
    {
        upperOpcode[i] = (char)toupper(upperOpcode[i]);
    }

    string baseOp, condition;
    bool hasS;
    parseOpcode(upperOpcode, baseOp, condition, hasS);

    bool condMet = checkCondition(condition);

    // --- MOV: Move value into destination register ---
    if (baseOp == "MOV")
    {
        // Format: MOV{cond}{S} Rd, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        string op2Token = stripComma(tokens[instrStart + 2]);

        if (condMet)
        {
            unsigned int value = 0;
            if (!op2Token.empty() && op2Token[0] == '#')
                value = parseImmediate(op2Token);
            else
                value = reg[parseRegIndex(op2Token)];

            reg[rd] = value;
            if (hasS) updateFlagsLogical(value);
        }
    }

    // --- MVN: Move bitwise NOT of value into destination register ---
    else if (baseOp == "MVN")
    {
        // Format: MVN{cond}{S} Rd, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        string op2Token = stripComma(tokens[instrStart + 2]);

        if (condMet)
        {
            unsigned int value = 0;
            if (!op2Token.empty() && op2Token[0] == '#')
                value = parseImmediate(op2Token);
            else
                value = reg[parseRegIndex(op2Token)];

            reg[rd] = ~value;
            if (hasS) updateFlagsLogical(reg[rd]);
        }
    }

    // --- CMP: Compare Rn and OP2, update flags without storing result ---
    else if (baseOp == "CMP")
    {
        // Format: CMP Rn, OP2
        // CMP always updates flags regardless of S suffix
        int rn = parseRegIndex(stripComma(tokens[instrStart + 1]));
        string op2Token = stripComma(tokens[instrStart + 2]);

        unsigned int op2Value = 0;
        if (!op2Token.empty() && op2Token[0] == '#')
            op2Value = parseImmediate(op2Token);
        else
            op2Value = reg[parseRegIndex(op2Token)];

        unsigned int rnVal = reg[rn];
        unsigned long long result64 = (unsigned long long)rnVal - (unsigned long long)op2Value;
        updateFlagsArithmetic(result64, rnVal, op2Value, true);
    }

    // --- ADD: Add Rn and OP2, store result in Rd ---
    else if (baseOp == "ADD")
    {
        // Format: ADD{S}{cond} Rd, Rn, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        string op2Token = stripComma(tokens[instrStart + 3]);

        unsigned int op2Value = 0;
        if (!op2Token.empty() && op2Token[0] == '#')
            op2Value = parseImmediate(op2Token);
        else
            op2Value = reg[parseRegIndex(op2Token)];

        if (condMet)
        {
            // Save rnVal before writing to rd (in case rd and rn are the same register)
            unsigned int rnVal = reg[rn];
            unsigned long long result64 = (unsigned long long)rnVal + (unsigned long long)op2Value;
            reg[rd] = (unsigned int)(result64 & 0xFFFFFFFF);
            if (hasS) updateFlagsArithmetic(result64, rnVal, op2Value, false);
        }
    }

    // --- SUB: Subtract OP2 from Rn, store result in Rd ---
    else if (baseOp == "SUB")
    {
        // Format: SUB{S}{cond} Rd, Rn, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        string op2Token = stripComma(tokens[instrStart + 3]);

        unsigned int op2Value = 0;
        if (!op2Token.empty() && op2Token[0] == '#')
            op2Value = parseImmediate(op2Token);
        else
            op2Value = reg[parseRegIndex(op2Token)];

        if (condMet)
        {
            unsigned int rnVal = reg[rn];
            unsigned long long result64 = (unsigned long long)rnVal - (unsigned long long)op2Value;
            reg[rd] = (unsigned int)(result64 & 0xFFFFFFFF);
            if (hasS) updateFlagsArithmetic(result64, rnVal, op2Value, true);
        }
    }

    // --- AND: Bitwise AND of Rn and OP2, store result in Rd ---
    else if (baseOp == "AND")
    {
        // Format: AND{S}{cond} Rd, Rn, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        string op2Token = stripComma(tokens[instrStart + 3]);

        unsigned int op2Value = 0;
        if (!op2Token.empty() && op2Token[0] == '#')
            op2Value = parseImmediate(op2Token);
        else
            op2Value = reg[parseRegIndex(op2Token)];

        if (condMet)
        {
            reg[rd] = reg[rn] & op2Value;
            if (hasS) updateFlagsLogical(reg[rd]);
        }
    }

    // --- ORR: Bitwise OR of Rn and OP2, store result in Rd ---
    else if (baseOp == "ORR")
    {
        // Format: ORR{S}{cond} Rd, Rn, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        string op2Token = stripComma(tokens[instrStart + 3]);

        unsigned int op2Value = 0;
        if (!op2Token.empty() && op2Token[0] == '#')
            op2Value = parseImmediate(op2Token);
        else
            op2Value = reg[parseRegIndex(op2Token)];

        if (condMet)
        {
            reg[rd] = reg[rn] | op2Value;
            if (hasS) updateFlagsLogical(reg[rd]);
        }
    }

    // --- EOR: Bitwise XOR of Rn and OP2, store result in Rd ---
    else if (baseOp == "EOR")
    {
        // Format: EOR{S}{cond} Rd, Rn, OP2
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        string op2Token = stripComma(tokens[instrStart + 3]);

        unsigned int op2Value = 0;
        if (!op2Token.empty() && op2Token[0] == '#')
            op2Value = parseImmediate(op2Token);
        else
            op2Value = reg[parseRegIndex(op2Token)];

        if (condMet)
        {
            reg[rd] = reg[rn] ^ op2Value;
            if (hasS) updateFlagsLogical(reg[rd]);
        }
    }

    // --- LSL: Shift Rn left by Imm bits, store result in Rd ---
    else if (baseOp == "LSL")
    {
        // Format: LSL{S}{cond} Rd, Rn, #Imm
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        unsigned int shiftAmt = parseImmediate(stripComma(tokens[instrStart + 3]));

        if (condMet)
        {
            reg[rd] = reg[rn] << shiftAmt;
            if (hasS) updateFlagsLogical(reg[rd]);
        }
    }

    // --- LSR: Shift Rn right by Imm bits, store result in Rd ---
    else if (baseOp == "LSR")
    {
        // Format: LSR{S}{cond} Rd, Rn, #Imm
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        int rn = parseRegIndex(stripComma(tokens[instrStart + 2]));
        unsigned int shiftAmt = parseImmediate(stripComma(tokens[instrStart + 3]));

        if (condMet)
        {
            reg[rd] = reg[rn] >> shiftAmt;
            if (hasS) updateFlagsLogical(reg[rd]);
        }
    }

    // --- LDR: Load a value from memory into Rd ---
    else if (baseOp == "LDR")
    {
        // Format: LDR{cond} Rd, [Rn]
        // The address to read from is stored in register Rn
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        string rnToken = stripBrackets(stripComma(tokens[instrStart + 2]));
        int rn = parseRegIndex(rnToken);

        if (condMet)
        {
            unsigned int address = reg[rn];
            int memIndex = addressToIndex(address);
            if (memIndex >= 0 && memIndex < 5)
            {
                reg[rd] = mem[memIndex];
            }
            else
            {
                cerr << "LDR: address 0x" << hex << address
                          << " is out of range" << dec << endl;
            }
        }
    }

    // --- STR: Store Rd's value into memory at address in Rn ---
    else if (baseOp == "STR")
    {
        // Format: STR{cond} Rd, [Rn]
        // The address to write to is stored in register Rn
        int rd = parseRegIndex(stripComma(tokens[instrStart + 1]));
        string rnToken = stripBrackets(stripComma(tokens[instrStart + 2]));
        int rn = parseRegIndex(rnToken);

        if (condMet)
        {
            unsigned int address = reg[rn];
            int memIndex = addressToIndex(address);
            if (memIndex >= 0 && memIndex < 5)
            {
                mem[memIndex] = reg[rd];
                memWritten[memIndex] = true;
            }
            else
            {
                cerr << "STR: address 0x" << hex << address
                          << " is out of range" << dec << endl;
            }
        }
    }

    // Print state after this instruction if requested
    if (doPrint)
    {
        printAll(rawLine);
    }
}


// -------------------------------------------------------
// Main function: reads the input file, builds a label map,
// then runs through each line executing instructions
// -------------------------------------------------------
int main()
{
    // Open the input file
    ifstream inFile("PP3_input.txt");
    if (!inFile.is_open())
    {
        cerr << "Error: Could not open PP3_input.txt" << endl;
        return 1;
    }

    // Read all lines into a vector (we need random access for BEQ branching)
    vector<string> lines;
    string line;
    while (getline(inFile, line))
    {
        lines.push_back(line);
    }
    inFile.close();

    // -------------------------------------------------------
    // Build a label map so BEQ can jump to the right line.
    // A line has a label if its first token is not a known opcode.
    // Example: "SKIP  SUB R3, R3, #1" -> label "SKIP" at that index
    // -------------------------------------------------------
    vector<pair<string, int>> labelMap;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        // Strip leading whitespace
        string raw = lines[i];
        int s = 0;
        while (s < (int)raw.size() && (raw[s] == ' ' || raw[s] == '\t')) s++;
        string trimmed = raw.substr(s);
        if (trimmed.empty()) continue;

        istringstream ss(trimmed);
        string firstToken;
        ss >> firstToken;

        string upper = firstToken;
        for (int j = 0; j < (int)upper.size(); j++)
        {
            upper[j] = (char)toupper(upper[j]);
        }

        // If the first token is not a known opcode, it is a label
        if (!isOpcodeStart(upper))
        {
            labelMap.push_back({upper, i});
        }
    }


    // -------------------------------------------------------
    // Main execution loop
    // pc (program counter) steps through the lines array
    // -------------------------------------------------------
    int pc = 0;

    while (pc < (int)lines.size())
    {
        string rawLine = lines[pc];
        pc++;

        // Strip inline comments (anything after ';')
        string noComment = rawLine;
        size_t semiPos = noComment.find(';');
        if (semiPos != string::npos)
        {
            noComment = noComment.substr(0, semiPos);
        }

        // Trim leading and trailing whitespace
        int s = 0;
        while (s < (int)noComment.size() && (noComment[s] == ' ' || noComment[s] == '\t')) s++;
        int e = (int)noComment.size() - 1;
        while (e >= 0 && (noComment[e] == ' ' || noComment[e] == '\t')) e--;
        if (s > e) continue;

        string trimmed = noComment.substr(s, e - s + 1);
        if (trimmed.empty()) continue;

        // Replace all commas with spaces so "R0,#0x14" becomes "R0 #0x14"
        // This makes tokenizing uniform regardless of spacing in the input
        string spacedLine = trimmed;
        for (int i = 0; i < (int)spacedLine.size(); i++)
        {
            if (spacedLine[i] == ',') spacedLine[i] = ' ';
        }

        // Tokenize by whitespace
        istringstream ss(spacedLine);
        vector<string> tokens;
        string tok;
        while (ss >> tok)
        {
            tokens.push_back(tok);
        }
        if (tokens.empty()) continue;

        // Determine if the first token is a label or an opcode
        string firstUpper = tokens[0];
        for (int i = 0; i < (int)firstUpper.size(); i++)
        {
            firstUpper[i] = (char)toupper(firstUpper[i]);
        }

        // instrStart = index in tokens[] where the actual opcode lives
        int instrStart = 0;
        if (!isOpcodeStart(firstUpper) && (int)tokens.size() > 1)
        {
            instrStart = 1;
        }

        if (instrStart >= (int)tokens.size()) continue;

        // Read and parse the opcode
        string opcodeRaw = tokens[instrStart];
        string upperOpcode = opcodeRaw;
        for (int i = 0; i < (int)upperOpcode.size(); i++)
        {
            upperOpcode[i] = (char)toupper(upperOpcode[i]);
        }

        string baseOp, condition;
        bool hasS;
        parseOpcode(upperOpcode, baseOp, condition, hasS);


        // -------------------------------------------------------
        // BEQ: Branch to label if Z == 1 (Equal condition is set)
        // Extra credit instruction.
        // Format: BEQ LABEL
        // -------------------------------------------------------
        if (baseOp == "BEQ")
        {
            string labelToken = tokens[instrStart + 1];
            string upperLabel = labelToken;
            for (int i = 0; i < (int)upperLabel.size(); i++)
            {
                upperLabel[i] = (char)toupper(upperLabel[i]);
            }

            // Always print the BEQ instruction and current state
            printAll(rawLine);

            // If the Equal flag is set, branch is taken
            if (flagZ == 1)
            {
                // Find the line index for this label
                int targetLineIndex = -1;
                for (int i = 0; i < (int)labelMap.size(); i++)
                {
                    if (labelMap[i].first == upperLabel)
                    {
                        targetLineIndex = labelMap[i].second;
                        break;
                    }
                }

                if (targetLineIndex == -1)
                {
                    cerr << "BEQ: label not found: " << labelToken << endl;
                    continue;
                }

                // Jump pc to the target label line.
                // Lines between BEQ and the label are skipped (not executed, not printed).
                // The label line itself will be executed in the next loop iteration.
                pc = targetLineIndex;
            }
            // If branch not taken (Z == 0), pc already points to the next line
            // (the ADD instruction), which will execute normally.
        }


        // -------------------------------------------------------
        // All other instructions: call executeInstruction
        // -------------------------------------------------------
        else
        {
            executeInstruction(tokens, instrStart, rawLine, true);
        }

    }

    return 0;
}