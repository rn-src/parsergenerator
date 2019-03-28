const int START = 17;
const int e = 18;
const int PROD_0 = 19;
const int PROD_1 = 20;
const int PROD_2 = 21;
const int PROD_3 = 22;
const int PROD_4 = 23;
const int PROD_5 = 24;
const int PROD_6 = 25;
const int PROD_7 = 26;
const int PROD_8 = 27;
const int nstates = 20;
static const int actions[] = {
/* state 0 */ ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,5,6,PROD_1,PROD_2,PROD_3,PROD_4,PROD_5,PROD_8,ACTION_SHIFT,6,2,PROD_6,PROD_7
/* state 1 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,7,4,PROD_1,PROD_2,PROD_3,PROD_8
/* state 2 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,8,4,PROD_1,PROD_2,PROD_3,PROD_8
/* state 3 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,9,6,PROD_1,PROD_2,PROD_3,PROD_4,PROD_5,PROD_8,ACTION_SHIFT,10,2,PROD_6,PROD_7
/* state 4 */ ,ACTION_REDUCE,PROD_8,1,6,-1,TIMES,DIV,MINUS,PLUS,RPAREN
/* state 5 */ ,ACTION_SHIFT,11,1,TIMES,ACTION_SHIFT,12,1,DIV,ACTION_SHIFT,13,1,MINUS,ACTION_SHIFT,14,1,PLUS,ACTION_STOP,PROD_0,1,1,-1
/* state 6 */ ,ACTION_SHIFT,13,1,MINUS,ACTION_SHIFT,14,1,PLUS,ACTION_STOP,PROD_0,1,1,-1
/* state 7 */ ,ACTION_REDUCE,PROD_3,2,6,-1,TIMES,DIV,MINUS,PLUS,RPAREN
/* state 8 */ ,ACTION_REDUCE,PROD_2,2,6,-1,TIMES,DIV,MINUS,PLUS,RPAREN
/* state 9 */ ,ACTION_SHIFT,11,1,TIMES,ACTION_SHIFT,12,1,DIV,ACTION_SHIFT,13,1,MINUS,ACTION_SHIFT,14,1,PLUS,ACTION_SHIFT,15,1,RPAREN
/* state 10 */ ,ACTION_SHIFT,13,1,MINUS,ACTION_SHIFT,14,1,PLUS,ACTION_SHIFT,15,1,RPAREN
/* state 11 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,16,4,PROD_1,PROD_2,PROD_3,PROD_8
/* state 12 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,17,4,PROD_1,PROD_2,PROD_3,PROD_8
/* state 13 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,18,6,PROD_1,PROD_2,PROD_3,PROD_4,PROD_5,PROD_8
/* state 14 */ ,ACTION_SHIFT,1,1,MINUS,ACTION_SHIFT,2,1,PLUS,ACTION_SHIFT,3,1,LPAREN,ACTION_SHIFT,4,1,NUMBER,ACTION_SHIFT,19,6,PROD_1,PROD_2,PROD_3,PROD_4,PROD_5,PROD_8
/* state 15 */ ,ACTION_REDUCE,PROD_1,3,6,-1,TIMES,DIV,MINUS,PLUS,RPAREN
/* state 16 */ ,ACTION_REDUCE,PROD_4,3,6,-1,TIMES,DIV,MINUS,PLUS,RPAREN
/* state 17 */ ,ACTION_REDUCE,PROD_5,3,6,-1,TIMES,DIV,MINUS,PLUS,RPAREN
/* state 18 */ ,ACTION_SHIFT,11,1,TIMES,ACTION_SHIFT,12,1,DIV,ACTION_REDUCE,PROD_7,3,4,-1,MINUS,PLUS,RPAREN
/* state 19 */ ,ACTION_SHIFT,11,1,TIMES,ACTION_SHIFT,12,1,DIV,ACTION_REDUCE,PROD_6,3,4,-1,MINUS,PLUS,RPAREN};
static const int actionstart[] = {0,30,53,76,106,116,137,150,160,170,190,202,225,248,273,298,308,318,328,344,360};
struct stack_t {
String str;
double tdouble;
};
static bool reduce(int productionidx, stack_t* inputs, stack_t& output){
switch(productionidx) {
case PROD_0:{ fprintf(stdout, "%f",inputs[0].tdouble); }
break;
case PROD_1:{ output.tdouble = inputs[1].tdouble; }
break;
case PROD_2:{ output.tdouble = inputs[1].tdouble; }
break;
case PROD_3:{ output.tdouble = -inputs[1].tdouble; }
break;
case PROD_4:{ output.tdouble = inputs[0].tdouble * inputs[2].tdouble; }
break;
case PROD_5:{ output.tdouble = inputs[0].tdouble / inputs[2].tdouble; }
break;
case PROD_6:{ output.tdouble = inputs[0].tdouble + inputs[2].tdouble; }
break;
case PROD_7:{ output.tdouble = inputs[0].tdouble - inputs[2].tdouble; }
break;
case PROD_8:{ output.tdouble = atof(inputs[0].str.c_str()); }
break;
default:
  break;
} /* end switch */
return true;
}