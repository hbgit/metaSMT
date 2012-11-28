#pragma once 

#include "../../tags/QF_BV.hpp"
#include "../../tags/Array.hpp"
#include "../../result_wrapper.hpp"

#include <iostream>
#include <map>
#include <stack>

#include <boost/spirit/include/support_utree.hpp>
#include "boost/lexical_cast.hpp"

namespace metaSMT {
namespace evaluator {
using namespace boost::spirit;

template<typename Context>
struct UTreeEvaluator
{
  enum smt2Symbol
  {
    undefined, setlogic, setoption, checksat, assertion, declarefun, getvalue, push, pop, exit
  };

  // TODO: check which bv constant/length/shift/comparison operators exist in SMT2
  enum smt2operator
  {
    other, smttrue, smtfalse, smtnot, smteq, smtand, smtor, smtxor, smtimplies, smtite, smtbvnot, smtbvand, smtbvor, smtbvxor, smtbvcomp, smtbvadd, smtbvmul, smtbvsub, smtbvdiv, smtbvrem
  };

  template<typename T1>
  struct result
  {
    typedef boost::spirit::utree::list_type type;
  };

  typedef std::map<std::string, boost::function<bool(boost::spirit::utree::list_type&)> > CommandMap;
  typedef std::map<std::string, smt2Symbol> SymbolMap;
  typedef std::map<std::string, smt2operator> OperatorMap;
  typedef BtorExp* result_type;

  UTreeEvaluator(Context& ctx) :
      ctx(ctx)
  {
  }

  boost::spirit::utree::list_type& operator()(boost::spirit::utree::list_type& ast) const
  {

    assert( ast.which() == utree::list_type());
    std::cout << ast << std::endl;
//        std::string command_name = ast.begin();
    ast.clear();
    return ast;
  }

  void initialize()
  {
    symbolMap["set-logic"] = setlogic;
    symbolMap["set-option"] = setoption;
    symbolMap["check-sat"] = checksat;
    symbolMap["assertion"] = assertion;
    symbolMap["assert"] = assertion;
    symbolMap["declare-fun"] = declarefun;
    symbolMap["get-value"] = getvalue;
    symbolMap["push"] = push;
    symbolMap["pop"] = pop;
    symbolMap["exit"] = exit;

    operatorMap["true"] = smttrue;
    operatorMap["false"] = smtfalse;
    operatorMap["not"] = smtnot;
    operatorMap["="] = smteq;
    operatorMap["and"] = smtand;
    operatorMap["or"] = smtor;
    operatorMap["xor"] = smtxor;
    operatorMap["=>"] = smtimplies;
    operatorMap["ite"] = smtite;
    operatorMap["bvnot"] = smtbvnot;
    operatorMap["bvand"] = smtbvand;
    operatorMap["bvor"] = smtbvor;
    operatorMap["bvxor"] = smtbvxor;
    operatorMap["bvcomp"] = smtbvcomp;
    operatorMap["bvadd"] = smtbvadd;
    operatorMap["bvmul"] = smtbvmul;
    operatorMap["bvsub"] = smtbvsub;
    operatorMap["bvdiv"] = smtbvdiv;
    operatorMap["bvrem"] = smtbvrem;

  }

  void print(boost::spirit::utree ast)
  {
    initialize();
    parseSymbolToString(ast);
    std::cout << metaSMTString << std::endl;
  }

  void solve(boost::spirit::utree ast)
  {
    initialize();
    parseSymbolToResultType(ast);
  }

  void parseSymbolToResultType(boost::spirit::utree ast)
  {
    bool pushed = false;
    for (boost::spirit::utree::iterator I = ast.begin(); I != ast.end(); ++I) {
      boost::spirit::utree command = *I;
      boost::spirit::utree::iterator commandIterator = command.begin();
      boost::spirit::utree symbol = *commandIterator;
      std::string symbolString = utreeToString(symbol);

      switch (symbolMap[symbolString]) {
      case push:
        pushed = true;
        break;
      case checksat:
        pushed = false;
        std::cout << ctx.solve() << std::endl;
        break;
      case assertion: {
        ++commandIterator;
        boost::spirit::utree logicalInstruction = *commandIterator;
//        std::cout << "logicalInstruction: " << logicalInstruction << std::endl;
        if (pushed) {
          ctx.assumption(translateLogicalInstructionToResultType(logicalInstruction));
        } else {
          ctx.assertion(translateLogicalInstructionToResultType(logicalInstruction));
        }
        break;
      }
      case declarefun:
      case getvalue:
      case setoption:
      case setlogic:
      case pop:
      case exit:
      case undefined:
      default:
        break;
      }
    }
  }

  result_type translateLogicalInstructionToResultType(boost::spirit::utree tree)
  {
    result_type output;
    switch (tree.which()) {
    case boost::spirit::utree::type::list_type: {
      for (boost::spirit::utree::iterator I = tree.begin(); I != tree.end(); ++I) {
        std::string value = utreeToString(*I);
////        std::cout << "value= " << value << std::endl;
        if (isOperator(value)) {
          operatorStack.push(value);
        } else {
//          // create var_tags
          if (value.compare("_") == 0) {
            ++I;
            ++I;
            std::string bitSize = utreeToString(*I);
            metaSMT::logic::QF_BV::tag::var_tag tag;
            unsigned width = boost::lexical_cast<unsigned>(bitSize);
            tag.width = width;
            resultTypeStack.push(ctx(tag));
          } else {
            metaSMT::logic::tag::var_tag tag;
            resultTypeStack.push(ctx(tag));
          }
        }
////        std::cout << "before: " << "operand= " << operandStack.size() << " operator= " << operatorStack.size() << std::endl;
//        consumeToString();
////        std::cout << "after: " << "operand= " << operandStack.size() << " operator= " << operatorStack.size() << std::endl;
      }
//      output += operandStack.top();
//      operandStack.pop();
//      break;
    }
    case boost::spirit::utree::type::string_type: {
      std::string value = utreeToString(tree);
      if (isOperator(value)) {
        operatorStack.push(value);
      } else {
        metaSMT::logic::tag::var_tag tag;
        resultTypeStack.push(ctx(tag));
      }
      constumeToResultType();
    }
    default:
      break;
    }
    output = resultTypeStack.top();
    resultTypeStack.pop();
    return output;
  }

  void constumeToResultType()
  {
    if (!operatorStack.empty()) {
      std::string op = operatorStack.top();
//      std::cout << "op= " << op << std::endl;
      switch (operatorMap[op]) {
      // constants
      case smttrue: {
        metaSMT::logic::tag::true_tag tag;
        resultTypeStack.push(ctx(tag));
        operatorStack.pop();
        constumeToResultType();
      }
        break;
      case smtfalse: {
        metaSMT::logic::tag::false_tag tag;
        resultTypeStack.push(ctx(tag));
        operatorStack.pop();
        constumeToResultType();
      }
        break;
        // unary operators
      case smtnot:
      case smtbvnot:
        break;
        // binary operators
      case smteq:
      case smtand:
      case smtor:
      case smtxor:
      case smtimplies:
      case smtbvand:
      case smtbvor:
      case smtbvxor:
      case smtbvcomp:
      case smtbvadd:
      case smtbvmul:
      case smtbvsub:
      case smtbvdiv:
      case smtbvrem:
        break;
        // ternary operators
      case smtite:
        break;
      case other:
      default:
//        std::cout << "couldn't recognize operator:" << op << std::endl;
        break;
      }
    }
  }

  void parseSymbolToString(boost::spirit::utree ast)
  {
    bool pushed = false;
    for (boost::spirit::utree::iterator I = ast.begin(); I != ast.end(); ++I) {
      boost::spirit::utree command = *I;
      boost::spirit::utree::iterator commandIterator = command.begin();
      boost::spirit::utree symbol = *commandIterator;
      std::string symbolString = utreeToString(symbol);

      switch (symbolMap[symbolString]) {
      case checksat:
        pushed = false;
        metaSMTString += "BOOST_REQUIRE( solve(ctx) );\n";
        break;
      case assertion: {
        ++commandIterator;
        boost::spirit::utree logicalInstruction = *commandIterator;
//        std::cout << "logicalInstruction: " << logicalInstruction << std::endl;
        if (pushed) {
          metaSMTString += nestString("assumption ( ctx, ", translateLogicalInstructionToString(logicalInstruction), ");\n");
        } else {
          metaSMTString += nestString("assertion ( ctx, ", translateLogicalInstructionToString(logicalInstruction), ");\n");
        }
        break;
      }
      case push: {
        pushed = true;
        break;
      }
      case declarefun: {
        metaSMTString += translateDeclareFunctionToString(command);
        break;
      }
      case getvalue: {
        ++commandIterator;
        boost::spirit::utree valueName = *commandIterator;
        metaSMTString += nestString("read_value(ctx,", utreeToString(valueName), ");\n");
        break;
      }
      case setoption:
      case setlogic:
      case pop:
      case exit:
      case undefined:
      default:
        break;
      }
    }
  }

  std::string translateDeclareFunctionToString(boost::spirit::utree function)
  {
    // (declare-fun var_1 () (_ BitVec 8))
    // bitvector x = new_bitvector(8);
    // (declare-fun var_1 () Bool)
    // predicate x = new_variable();
    boost::spirit::utree::iterator functionIterator = function.begin();
    ++functionIterator;
    std::string functionName = utreeToString(*functionIterator);
    ++functionIterator;
    ++functionIterator;
    boost::spirit::utree functionType = *functionIterator;
    std::string output = "";

    switch (functionType.which()) {
    case boost::spirit::utree::type::list_type: {
      boost::spirit::utree::iterator bitVecIterator = functionType.begin();
      ++bitVecIterator;
      ++bitVecIterator;
      std::string bitSize = utreeToString(*bitVecIterator);
      output = "bitvector " + functionName + " = new_bitvector(" + bitSize + ");\n";
      break;
    }
    case boost::spirit::utree::type::string_type: {
      output = "predicate " + functionName + " = new_variable();\n";
      break;
    }
    default:
      break;
    }
    return output;
  }

  std::string translateLogicalInstructionToString(boost::spirit::utree tree)
  {
    std::string output = "";
    switch (tree.which()) {
    case boost::spirit::utree::type::list_type: {
      for (boost::spirit::utree::iterator I = tree.begin(); I != tree.end(); ++I) {
        std::string value = utreeToString(*I);
//        std::cout << "value= " << value << std::endl;
        if (isOperator(value)) {
          operatorStack.push(value);
        } else {
          // handle s.th. like ((_ bv123 32) a)
          if (value.compare("_") == 0) {
            ++I;
            std::string nextToken = utreeToString(*I);
            nextToken.erase(0, 2);
            ++I;
            std::string bitSize = utreeToString(*I);
            std::string operand = "bvsint(" + nextToken + "," + bitSize + ")";
            operandStack.push(operand);
          } else {
            operandStack.push(value);
          }
        }
//        std::cout << "before: " << "operand= " << operandStack.size() << " operator= " << operatorStack.size() << std::endl;
        consumeToString();
//        std::cout << "after: " << "operand= " << operandStack.size() << " operator= " << operatorStack.size() << std::endl;
      }
      output += operandStack.top();
      operandStack.pop();
      break;
    }
    case boost::spirit::utree::type::string_type: {
      std::string value = utreeToString(tree);
      output += value;
      break;
    }
    default:
      break;
    }
    return output;
  }

  void consumeToString()
  {
    if (!operatorStack.empty()) {
      std::string op = operatorStack.top();
//      std::cout << "op= " << op << std::endl;
      switch (operatorMap[op]) {
      // constants
      case smttrue:
      case smtfalse: {
        std::string newOperand = translateLogicalOeratorToString(op);
        operandStack.push(newOperand);
        operatorStack.pop();
        consumeToString();
      }
        break;
        // unary operators
      case smtnot:
      case smtbvnot:
        if (operandStack.size() > 0) {
          std::string op1 = operandStack.top();
          operandStack.pop();
          std::string newOperand = translateLogicalOeratorToString(op) + op1 + ")";
          operandStack.push(newOperand);
          operatorStack.pop();
        }
        break;
        // binary operators
      case smteq:
      case smtand:
      case smtor:
      case smtxor:
      case smtimplies:
      case smtbvand:
      case smtbvor:
      case smtbvxor:
      case smtbvcomp:
      case smtbvadd:
      case smtbvmul:
      case smtbvsub:
      case smtbvdiv:
      case smtbvrem:
        if (operandStack.size() >= 2) {
          std::string op2 = operandStack.top();
          operandStack.pop();
          std::string op1 = operandStack.top();
          operandStack.pop();
          std::string newOperand = translateLogicalOeratorToString(op) + op1 + "," + op2 + ")";
          operandStack.push(newOperand);
          operatorStack.pop();
        }
        break;
        // ternary operators
      case smtite:
        if (operandStack.size() >= 3) {
          std::string op3 = operandStack.top();
          operandStack.pop();
          std::string op2 = operandStack.top();
          operandStack.pop();
          std::string op1 = operandStack.top();
          operandStack.pop();
          std::string newOperand = translateLogicalOeratorToString(op) + op1 + "," + op2 + "," + op3 + ")";
          operandStack.push(newOperand);
          operatorStack.pop();
        }
        break;
      case other:
      default:
        std::cout << "couldn't recognize operator:" << op << std::endl;
        break;
      }
    }
  }

  bool isOperator(std::string op)
  {
    switch (operatorMap[op]) {
    case smttrue:
    case smtfalse:
    case smteq:
    case smtnot:
    case smtand:
    case smtor:
    case smtxor:
    case smtimplies:
    case smtite:
    case smtbvand:
    case smtbvor:
    case smtbvxor:
    case smtbvcomp:
    case smtbvadd:
    case smtbvmul:
    case smtbvsub:
    case smtbvdiv:
    case smtbvrem:
      return true;
    case other:
    default:
      break;
    }
    return false;
  }

  std::string utreeToString(boost::spirit::utree tree)
  {
    std::stringstream stringStream;
    stringStream << tree;
    std::string output = stringStream.str();
    size_t found = output.find("\"");
    while (found != output.npos) {
      output.erase(found, 1);
      found = output.find("\"");
    }
    found = output.find(" ");
    while (found != output.npos) {
      output.erase(found, 1);
      found = output.find(" ");
    }
    if (output.size() > 2) {
      if (output.find("#", 0, 1) != output.npos) {
        if (output.find("b", 1, 1) != output.npos) {
          output.erase(0, 2);
          output = "bvbin(\"" + output + "\")";
        } else if (output.find("x", 1, 1) != output.npos) {
          output.erase(0, 2);
          output = "bvhex(\"" + output + "\")";
        }
      }
    }
    return output;
  }

  std::string translateLogicalOeratorToString(std::string op)
  {
    switch (operatorMap[op]) {
    case smttrue:
      return "true";
    case smtfalse:
      return "false";
    case smteq:
      return "metaSMT::logic::equal(";
    case smtnot:
      return "Not(";
    case smtand:
      return "And(";
    case smtor:
      return "Or(";
    case smtxor:
      return "Xor(";
    case smtimplies:
      return "implies(";
    case smtite:
      return "Ite(";
    case smtbvnot:
      return "bvnot(";
    case smtbvand:
      return "bvand(";
    case smtbvor:
      return "bvor(";
    case smtbvxor:
      return "bvxor(";
    case smtbvcomp:
      return "bvcomp(";
    case smtbvadd:
      return "bvadd(";
    case smtbvmul:
      return "bvmul(";
    case smtbvsub:
      return "bvsub(";
    case smtbvdiv:
      return "bvdiv(";
    case smtbvrem:
      return "bvrem(";
    default:
      return "undefinedOperator";
    }
    return "";
  }

  std::string nestString(std::string pre, std::string nest, std::string post)
  {
    return pre + nest + post;
  }

private:
  Context& ctx;
  CommandMap commandMap;
  SymbolMap symbolMap;
  OperatorMap operatorMap;
  std::string metaSMTString;
  std::stack<std::string> operatorStack;
  std::stack<std::string> operandStack;
  std::stack<result_type> resultTypeStack;

};
// struct UTreeEvaluator
}// namespace evaluator
} // namespace metaSMT
