#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/Expr.h"
#include <string>
#include "clang/AST/Stmt.h"
#include <sstream>
#include <iostream>
#include <dlfcn.h>                                                                                                                                                                                                                                                                      
#include <unordered_map>                                                                                                                                                                                                                                                                 
#include <mutex>                                                                                                                                                                                                                                                                         
#include <string.h>
#include <tuple>

using namespace clang;

namespace {
  uint32_t get_func_hash(void *func);
// RecursiveASTVisitor does a pre-order depth-first traversal of the
// AST.
 
class FuncDeclVisitor : public RecursiveASTVisitor<FuncDeclVisitor> {
public:
   explicit FuncDeclVisitor(DiagnosticsEngine &d) : diagmsg(d) {}

  // This function gets called for each FunctionDecl node in the AST.
  // Returning true indicates that the traversal should continue.
  bool VisitFunctionDecl(const FunctionDecl *funcDecl) {
    
    std::string func =funcDecl->getNameInfo().getAsString();
    //DeclarationNameInfo *fp;

    if(funcDecl->isExternC()) {
      
      std::string ret = funcDecl->getReturnType().getAsString();
      std::cout <<"Extern Function return type is " << ret<< std::endl;
      std::cout <<"Extern Function name is "<< func<<std::endl ;
      
      get_func_hash(&func);
    
    }

    
     if (const FunctionDecl *prevDecl = funcDecl->getPreviousDecl()) {
       // If one of the declarations is without prototype, we can't compare them.


      if (!funcDecl->hasPrototype() || !prevDecl->hasPrototype())
        return true;

      assert(funcDecl->getNumParams() == prevDecl->getNumParams());

      for (unsigned i = 0, e = funcDecl->getNumParams(); i != e; ++i) {
        const ParmVarDecl *paramDecl = funcDecl->getParamDecl(i);
        const ParmVarDecl *previousParamDecl = prevDecl->getParamDecl(i);

        // Ignore the case of unnamed parameters.
        if (paramDecl->getName() == "" || previousParamDecl->getName() == "")
          return true;

        if (paramDecl->getIdentifier() != previousParamDecl->getIdentifier()) {
          unsigned warn = diagmsg.getCustomDiagID(DiagnosticsEngine::Warning,
              "parameter name mismatch");
          diagmsg.Report(paramDecl->getLocation(), warn);

          unsigned note = diagmsg.getCustomDiagID(DiagnosticsEngine::Note,
              "parameter in previous function declaration was here");
          diagmsg.Report(previousParamDecl->getLocation(), note);	    	  
        }
      }
    }


     return true;   
  } 
  
private:
   DiagnosticsEngine &diagmsg;
};

// An ASTConsumer is a client object that receives callbacks as the AST is
// built, and "consumes" it.
class FuncDeclConsumer : public ASTConsumer {
public:
  explicit FuncDeclConsumer(DiagnosticsEngine &d)
      : m_visitor(FuncDeclVisitor(d)) {}

  // Called by the parser for each top-level declaration group.
  // Returns true to continue parsing, or false to abort parsing.
  virtual bool HandleTopLevelDecl(DeclGroupRef dg) override {

    for (Decl *decl : dg) {
      m_visitor.TraverseDecl(decl);
    }

    return true;
  }

private:
  FuncDeclVisitor m_visitor;
};

  class CrossChecker : public PluginASTAction {
protected:
  // Create the ASTConsumer that will be used by this action.
  // The StringRef parameter is the current input filename (which we ignore).
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci,
                                                 llvm::StringRef) override {
    return llvm::make_unique<FuncDeclConsumer>(ci.getDiagnostics());
  }

  // Parse command-line arguments. Return true if parsing succeeded, and
  // the plugin should proceed; return false otherwise.
  bool ParseArgs(const CompilerInstance&,
                 const std::vector<std::string>&) override {
    // We don't care about command-line arguments.Might care later on
    return true;
  }
};


uint32_t djb2_hash(const uint8_t *str) {

  //std::cout << str ;
    uint32_t hash = 5381;                                                                                                                                                                                                                                                                 
    while (uint32_t c = static_cast<uint32_t>(*str++))                                                                                                                                                                                                                                    
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */                                                                                                                                                                                                                              
    return hash;                                                                                                                                                                                                                                                                          
}                                                                                                                                                                                                                                                                                        std::unordered_map<void*, uint32_t> hash_cache;                                                                                                                                                                                                                                          std::mutex cache_mutex;                                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                                                          uint32_t get_func_hash(void *func) {

  //std::cout << *func;
  std::string &s = *static_cast<std::string*>(func);
  std::lock_guard<std::mutex> lock(cache_mutex);                                                                                                                                                                                                                                           auto it = hash_cache.find(&s);                                                                                                                                                                                                                                                          if (it != hash_cache.end())                                                                                                                                                                                                                                                                return it->second;

  Dl_info func_info, caller_info;
  dladdr(&s, &func_info);
  auto *func_name = reinterpret_cast<const uint8_t*>(func_info.dli_sname);
  uint32_t func_hash = djb2_hash(func_name);
  hash_cache[&s] = func_hash;
  return func_hash;

} 

  
} 

// Register the PluginASTAction in the registry.
// This makes it available to be run with the '-plugin' command-line option.
static FrontendPluginRegistry::Add<CrossChecker>
X("cross-check", "check for parameter names mismatch");


  
