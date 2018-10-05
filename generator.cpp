/*
 * File:	generator.cpp
 *
 * Description:	This file contains the public and member function
 *		definitions for the code generator for Simple C.
 *
 *		Extra functionality:
 *		- putting all the global declarations at the end
 */

# include <sstream>
# include <iostream>
# include "generator.h"
# include "Register.h"
# include "machine.h"
# include "Tree.h"
# include <cassert>
# include "Label.h"
# include "tokens.h"

using namespace std;


/* This needs to be set to zero if temporaries are placed on the stack. */

# define SIMPLE_PROLOGUE 0


/* Okay, I admit it ... these are lame, but they work. */

# define isNumber(expr)		(expr->_operand[0] == '$')
# define isRegister(expr)	(expr->_register != nullptr)
# define isMemory(expr)		(!isNumber(expr) && !isRegister(expr))


/* The registers that we are using in the assignment. */

static Register *rax = new Register("%rax", "%eax", "%al");
static Register *rdi = new Register("%rdi", "%edi", "%dil");
static Register *rsi = new Register("%rsi", "%esi", "%sil");
static Register *rdx = new Register("%rdx", "%edx", "%dl");
static Register *rcx = new Register("%rcx", "%ecx", "%cl");
static Register *r8 = new Register("%r8", "%r8d", "%r8b");
static Register *r9 = new Register("%r9", "%r9d", "%r9b");
static Register *r10 = new Register("%r10", "%r10d", "%r10b");
static Register *r11 = new Register("%r11", "%r11d", "%r11b");

static Register *parameters[] = {rdi, rsi, rdx, rcx, r8, r9};
vector<Register *> registers = {r11, r10, r9, r8, rcx, rdx, rsi, rdi, rax};

int tempOffset;
Label *retLbl;
vector<string> stringLabels;


/*
 * Function:	suffix (private)
 *
 * Description:	Return the suffix for an opcode based on the given size.
 */

static string suffix(unsigned size)
{
    return size == 1 ? "b" : (size == 4 ? "l" : "q");
}


/*
 * Function:	align (private)
 *
 * Description:	Return the number of bytes necessary to align the given
 *		offset on the stack.
 */

static int align(int offset)
{
    if (offset % STACK_ALIGNMENT == 0)
	return 0;

    return STACK_ALIGNMENT - (abs(offset) % STACK_ALIGNMENT);
}


/*
 * Function:	operator << (private)
 *
 * Description:	Write an expression to the specified stream.  This function
 *		first checks to see if the expression is in a register, and
 *		if not then uses its operand.
 */

static ostream &operator <<(ostream &ostr, Expression *expr)
{
    if (expr->_register != nullptr)
	return ostr << expr->_register;

    return ostr << expr->_operand;
}


Register *getreg()
{
    for (unsigned i = 0; i < registers.size(); i ++)
        if (registers[i]->_node == nullptr)
            return registers[i];
    load(nullptr, registers[0]);
    return registers[0];
}

void release()
{
    for (unsigned i = 0; i < registers.size(); i ++)
        assign(nullptr, registers[i]);
}

void assign(Expression *expr, Register *reg)
{
    if (expr != nullptr) {
        if (expr->_register != nullptr)
            expr->_register->_node = nullptr;
        expr->_register = reg;
    }
    if (reg != nullptr) {
        if (reg->_node != nullptr)
            reg->_node->_register = nullptr;
        reg->_node = expr;
    }
}

void assigntemp(Expression *expr)
{
    stringstream ss;
    tempOffset = tempOffset - expr->type().size();
    ss << tempOffset << "(%rbp)";
    expr->_operand = ss.str();  
}

void load(Expression *expr, Register *reg)
{
    if (reg->_node != expr) {
        if (reg->_node != nullptr) {
            unsigned size = reg->_node->type().size();

            assigntemp(reg->_node);
            cout << "\tmov\t" << reg->name(size);
            cout << ", " << reg->_node->_operand;
            cout << "\t# spill" << endl;
        }

        if (expr != nullptr) {
            unsigned size = expr->type().size();
            cout << "\tmov\t" << expr << ", ";
            cout << reg->name(size) << endl;
        }   
        assign(expr, reg);
    }
}


/*
 * Function:	Number::generate
 *
 * Description:	Generate code for a number.  Since there is really no code
 *		to generate, we simply update our operand.
 */

void Number::generate()
{
    stringstream ss;

    ss << "$" << _value;
    _operand = ss.str();
}


/*
 * Function:	Identifier::generate
 *
 * Description:	Generate code for an identifier.  Since there is really no
 *		code to generate, we simply update our operand.
 */

void Identifier::generate()
{
    stringstream ss;


    if (_symbol->_offset == 0)
	ss << global_prefix << _symbol->name() << global_suffix;
    else
	ss << _symbol->_offset << "(%rbp)";

    _operand = ss.str();
}

void String::generate()
{
    stringstream ss;

    Label stringLbl;
    ss << stringLbl << ":\t.asciz\t" << _value;

    stringstream temp;
    temp << stringLbl;

    _operand = temp.str();
    stringLabels.push_back(ss.str());
}


void Expression::test(const Label &label, bool ifTrue)
{
    generate();
    if (_register == nullptr)
        load(this, getreg());
    cout << "\tcmp\t$0, " << this << endl;
    cout << (ifTrue ? "\tjne\t" : "\tje\t") << label << endl;
    assign(this, nullptr);
}

/*
 * Function:	Call::generate
 *
 * Description:	Generate code for a function call.  Arguments are first
 *		evaluated in case any them are in fact other function
 *		calls.  The first six arguments are placed in registers and
 *		any remaining arguments are pushed on the stack from right
 *		to left.  Each argument on the stack always requires eight
 *		bytes, so the stack will always be aligned on a multiple of
 *		eight bytes.  To ensure 16-byte alignment, we adjust the
 *		stack pointer if necessary.
 *
 *		NOT FINISHED: Ignores any return value.
 */

void Call::generate()
{
    unsigned size, bytesPushed = 0;

    /* Generate code for all the arguments first. */

    for (unsigned i = 0; i < _args.size(); i ++)
	   _args[i]->generate();

    for(int i = 0; i < registers.size(); i++)
        load(nullptr, registers[i]);

    /* Adjust the stack if necessary. */

    if (_args.size() > NUM_ARGS_IN_REGS) {
	bytesPushed = align((_args.size() - NUM_ARGS_IN_REGS) * SIZEOF_ARG);

	if (bytesPushed > 0)
	    cout << "\tsubq\t$" << bytesPushed << ", %rsp" << endl;
    }

    /* Move the arguments into the correct registers or memory locations. */

    for (int i = _args.size() - 1; i >= 0; i --) {
	size = _args[i]->type().size();

	if (i < NUM_ARGS_IN_REGS) {
	    cout << "\tmov" << suffix(size) << "\t" << _args[i] << ", ";
	    cout << parameters[i]->name(size) << endl;
	} else {
	    bytesPushed += SIZEOF_ARG;

	    if (isRegister(_args[i]))
		cout << "\tpushq\t" << _args[i]->_register->name() << endl;
	    else if (isNumber(_args[i]) || size == SIZEOF_ARG)
		cout << "\tpushq\t" << _args[i] << endl;
	    else {
		cout << "\tmov" << suffix(size) << "\t" << _args[i] << ", ";
		cout << rax->name(size) << endl;
		cout << "\tpushq\t%rax" << endl;
	    }
	}
    }


    /* Call the function.  Technically, we only need to assign the number
       of floating point arguments to %eax if the function being called
       takes a variable number of arguments.  But, it never hurts. */

    if (_id->type().parameters() == nullptr)
	cout << "\tmovl\t$0, %eax" << endl;

    cout << "\tcall\t" << global_prefix << _id->name() << endl;

    assign(this, rax);
    /* Reclaim the space of any arguments pushed on the stack. */

    if (bytesPushed > 0)
	cout << "\taddq\t$" << bytesPushed << ", %rsp" << endl;

}


/*
 * Function:	Assignment::generate
 *
 * Description:	Generate code for an assignment statement.
 *
 *		NOT FINISHED: Only works if the right-hand side is an
 *		integer literal and the left-hand size is an integer
 *		scalar.
 */

void Assignment::generate()
{
    cout << "#Assignment Start" << endl;

    if(_left->getDereference() == nullptr)
        _left->generate();
    else
        _left->getDereference()->generate();

    _right->generate();

    if (_right->_register == nullptr)
        load(_right, getreg());


    stringstream ss;

    if(_left->getDereference() == nullptr)
        ss << _left;
    else{
        if(_left->getDereference()->_register == nullptr)
            load(_left->getDereference(),getreg());
        ss << "(" <<_left->getDereference() << ")";
    }

    cout << "\tmov" << suffix(_left->type().size()) << "\t" << _right << ", " << ss.str() <<endl;

    cout << "#Assignment End" << endl;
}


/*
 * Function:	Block::generate
 *
 * Description:	Generate code for this block, which simply means we
 *		generate code for each statement within the block.
 */

void Block::generate()
{
    for (unsigned i = 0; i < _stmts.size(); i ++){
	   _stmts[i]->generate();
       release();
       cout << endl;
    }
}


void Return::generate()
{
    _expr -> generate();
    load(_expr, rax);
    cout << "\tjmp\t" << *retLbl << endl;
}

/*
 * Function:	Function::generate
 *
 * Description:	Generate code for this function, which entails allocating
 *		space for local variables, then emitting our prologue, the
 *		body of the function, and the epilogue.
 */

void Function::generate()
{
    int offset = 0;
    unsigned numSpilled = _id->type().parameters()->size();
    const Symbols &symbols = _body->declarations()->symbols();

    retLbl = new Label();

    /* Assign offsets to all symbols within the scope of the function. */

    allocate(offset);


    /* Generate the prologue, body, and epilogue. */

    cout << global_prefix << _id->name() << ":" << endl;
    cout << "\tpushq\t%rbp" << endl;
    cout << "\tmovq\t%rsp, %rbp" << endl;

    if (SIMPLE_PROLOGUE) {
	   offset -= align(offset);
	   cout << "\tsubq\t$" << -offset << ", %rsp" << endl;
    } else {
	   cout << "\tmovl\t$" << _id->name() << ".size, %eax" << endl;
	   cout << "\tsubq\t%rax, %rsp" << endl;
    }

    if (numSpilled > NUM_ARGS_IN_REGS)
	numSpilled = NUM_ARGS_IN_REGS;

    for (unsigned i = 0; i < numSpilled; i ++) {
	unsigned size = symbols[i]->type().size();
	cout << "\tmov" << suffix(size) << "\t" << parameters[i]->name(size);
	cout << ", " << symbols[i]->_offset << "(%rbp)" << endl;
    }

    tempOffset = offset; 
    _body->generate();
    offset = tempOffset;

    cout << *retLbl << ":" << endl; 

    cout << "\tmovq\t%rbp, %rsp" << endl;
    cout << "\tpopq\t%rbp" << endl;
    cout << "\tret" << endl << endl;


    /* Finish aligning the stack. */

    if (!SIMPLE_PROLOGUE) {
	offset -= align(offset);
	cout << "\t.set\t" << _id->name() << ".size, " << -offset << endl;
    }

    cout << "\t.globl\t" << global_prefix << _id->name() << endl << endl;
}


/*
 * Function:	generateGlobals
 *
 * Description:	Generate code for any global variable declarations.
 */

void generateGlobals(Scope *scope)
{
    const Symbols &symbols = scope->symbols();

    for (unsigned i = 0; i < symbols.size(); i ++)
	if (!symbols[i]->type().isFunction()) {
	    cout << "\t.comm\t" << global_prefix << symbols[i]->name() << ", ";
	    cout << symbols[i]->type().size() << endl;
	}
}

void generateStrings()
{
    for(int i = 0; i < stringLabels.size(); i++)
        cout << stringLabels[i] << endl;
}

void Add::generate()
{   
    cout << "#Add start" << endl;
    _left->generate();
    _right->generate();

    if (_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tadd\t" << _right << ", " << _left << endl;
    assign(_right, nullptr);
    assign(this, _left->_register);
    cout << "#Add end" << endl;
}

void Subtract::generate()
{
    cout << "#Subtract Start" << endl;
    _left->generate();
    _right->generate();

    if (_left->_register == nullptr)
        load(_left, getreg());
    cout << "\tsub\t" << _right << ", " << _left << endl;
    assign(_right, nullptr);
    assign(this, _left->_register);
    cout << "#Subtract End" << endl;
}


void Multiply::generate()
{
    cout << "#Multiply Start" << endl;
    _left->generate();
    _right->generate();
    
    if(_left->_register == nullptr)
        load(_left, getreg());
    cout << "\timul\t" << _right << ", " <<  _left << endl;
    assign(_right, nullptr);
    assign(this, _left->_register);
    cout << "#Multiply End" << endl;
}

void Divide::generate()
{
    cout << "#Divide Start" << endl;
    _left->generate();
    _right->generate();

    load(_left, rax);
    cout << "\tmovl\t$0, %edx" << endl;


    Register* tempReg = getreg();

    if(_right->type().specifier() == INT){
        cout << "\tcltd" << endl;
        if(isNumber(_right))
            load(_right, tempReg);
        cout << "\tidivl\t" << _right << endl;
    }
    else{
        cout << "\tcqto" << endl;
        if(isNumber(_right))
            load(_right, tempReg);
        cout << "\tidivq\t" << _right << endl;
    }

    assign(_right, nullptr);
    assign(this, rax);
    assign(nullptr, rdx);
    cout << "#Divide End" << endl;
}

void Remainder::generate()
{
    cout << "#Remainder Start" << endl;
    _left->generate();
    _right->generate();

    load(_left, rax);
    cout << "\tmovl\t$0, %edx" << endl;


    Register* tempReg = getreg();

    if(_right->type().specifier() == INT){
        cout << "\tcltd" << endl;
        if(isNumber(_right))
            load(_right, tempReg);
        cout << "\tidivl\t" << _right << endl;
    }
    else{
        cout << "\tcqto" << endl;
        if(isNumber(_right))
            load(_right, tempReg);
        cout << "\tidivq\t" << _right << endl;
    }

    assign(_right, nullptr);
    assign(this, rdx);
    assign(nullptr, rax);
    cout << "#Remainder End" << endl;
}

void Address::generate()
{
    cout << "#Address Start" << endl;

    if(_expr->getDereference() != nullptr)
    {
        _expr->getDereference()->generate();
        assign(this, _expr->getDereference()->_register);
        return;
    }

    _expr->generate();
 
    if(_expr->_register != nullptr)
        assign(this, _expr->_register);

    Register *tempReg = getreg();
    assign(this, tempReg);
    cout << "\tleaq\t" << _expr << "," << this << endl;

    cout << "#Address End" << endl;
}

void Negate::generate()
{
    cout <<"#Negate Start" << endl;
    _expr->generate();

    Register* tempReg = getreg();
    load(_expr, tempReg);
    cout << "\tneg" << suffix(_type.size()) << "\t" << _expr << endl;

    assign(this,_expr->_register);

    cout << "#Negate End" << endl;
}

void Cast::generate()
{
    cout << "#Cast Start" << endl;
    _expr->generate();


    if(_expr->type().size() >= _type.size())
    {
        _operand = _expr->_operand;
        assign(this, _expr->_register);
    }
    else
    {
        Register* tempReg = getreg();
        if(isNumber(_expr))
            load(_expr,tempReg);

        cout << "\tmovs" << suffix(_expr->type().size()) << suffix(_type.size()) << "\t" << _expr << ", " << tempReg->name(_type.size()) <<endl;

        assign(this, tempReg);       
    }
    cout << "#Cast End" << endl;
}

void Not::generate()
{
    cout<< "#Not Start" << endl;
    _expr->generate();

    Register* tempReg = getreg();
    load(_expr, tempReg);
    cout << "\tcmp" << suffix(_expr->type().size()) << "\t$0," << _expr << endl;
    cout << "\tsete\t" << _expr->_register->name(1) << endl;
    cout << "\tmovzbl\t" << _expr->_register->name(1) << ", " << _expr->_register->name(4) << endl;

    assign(this,_expr->_register);

    cout << "#Not End" << endl;
}

void Dereference::generate()
{
    cout << "#Dereference Start" << endl;
    _expr->generate();

    if(_expr->_register == nullptr)
        load(_expr,getreg());

    cout<< "\tmov" << suffix(_type.size()) << "\t(" << _expr << "), " << _expr->_register->name(_type.size()) << endl;
    assign(this,_expr->_register);
    cout << "#Dereference End" << endl;
}


void While::generate()
{
    cout << "#While Start" << endl;

    Label loop, exit;

    cout << loop << ":" << endl;

    _expr->test(exit, false);
    _stmt->generate();
    release();

    cout << "\tjmp\t" << loop << endl;
    cout << exit << ":" << endl;

    cout << "#While End" << endl;
}

void If::generate()
{
    cout << "#IF Start" << endl;

    Label next, exit;

    _expr->test(next, false);
    _thenStmt->generate();

    if(_elseStmt == nullptr)
        cout << next << ":" << endl;
    else
    {
        cout << "\tjmp\t" << exit << endl;
        cout << next << ":" << endl;
        _elseStmt->generate();
        cout << exit << ":" << endl;
    }

    cout << "#IF End" << endl;
}

void LessThan::generate()
{
    cout << "#Less Than Start" << endl;

    _left->generate();
    _right->generate();

    if(_left->_register == nullptr){
        Register* tempReg = getreg();
        load(_left, tempReg);
    }

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsetl\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(_right, nullptr);
    assign(this, _left->_register);

    cout << "#Less Than End" << endl;
}

void GreaterThan::generate()
{
    cout << "#Greater Than Start" << endl;

    _left->generate();
    _right->generate();

    if(_left->_register == nullptr){
        Register* tempReg = getreg();
        load(_left, tempReg);
    }

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsetg\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(_right, nullptr);
    assign(this, _left->_register);

    cout << "#Greater Than End" << endl;
}

void LessOrEqual::generate()
{
    cout << "#LessOrEqual Than Start" << endl;

    _left->generate();
    _right->generate();

    if(_left->_register == nullptr){
        Register* tempReg = getreg();
        load(_left, tempReg);
    }

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsetle\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(_right, nullptr);
    assign(this, _left->_register);

    cout << "#LessOrEqual Than End" << endl;
}


void GreaterOrEqual::generate()
{
    cout << "#GreaterOrEqual Than Start" << endl;

    _left->generate();
    _right->generate();

    if(_left->_register == nullptr){
        Register* tempReg = getreg();
        load(_left, tempReg);
    }

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsetge\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(_right, nullptr);
    assign(this, _left->_register);

    cout << "#GreaterOrEqual Than End" << endl;
}

void Equal::generate()
{
    cout << "#Equal Start" << endl;

    _left->generate();
    _right->generate();

    if(_left->_register == nullptr){
        Register* tempReg = getreg();
        load(_left, tempReg);
    }

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsete\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(_right, nullptr);
    assign(this, _left->_register);

    cout << "#Equal End" << endl;
}

void NotEqual::generate()
{
    cout << "#Not Equal Start" << endl;

    _left->generate();
    _right->generate();

    if(_left->_register == nullptr){
        Register* tempReg = getreg();
        load(_left, tempReg);
    }

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsetne\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(_right, nullptr);
    assign(this, _left->_register);

    cout << "#Not Equal End" << endl;
}

void LogicalOr::generate()
{
    cout << "#LogicalOr Start" << endl;

    Label L1, L2;

    _left->test(L1, true);
    _right->test(L1, true);

    Register* tempReg = getreg();
    cout << "\tmov\t $0, " << tempReg << endl;
    cout << "\tjmp\t" << L2 << endl;
    cout << L1 << ":" << endl;
    cout << "\tmov\t $1, " << tempReg << endl;
    cout << L2 << ":" << endl;

    assign(this, tempReg);

    cout << "#LogicalOr End" << endl;
}

void LogicalAnd::generate()
{  
    cout << "#LogicalAnd Start" << endl;

    Label L1, L2;

    _left->test(L1, false);
    _right->test(L1, false);

    Register* tempReg = getreg();
    cout << "\tmov\t $0, " << tempReg << endl;
    cout << "\tjmp\t" << L2 << endl;
    cout << L1 << ":" << endl;
    cout << "\tmov\t $1, " << tempReg << endl;
    cout << L2 << ":" << endl;

    assign(this, tempReg);

    cout << "#LogicalAnd End" << endl;
}






