#include <bobopt_config.hpp>
#include <bobopt_optimizer.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <cstdarg>
#include <memory>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace bobopt
{

    // optimizer_frontend_action_factory/optimizer_frontend_action implementation.
    //==============================================================================

    /// \brief A little wrapping to catch \c clang::CompilerInstance so we can access \c clang::Sema.
    /// There's no other possibility to access those objects from code inside match finder handling
    /// member function.
    template <typename FactoryT>
    class optimizer_frontend_action_factory : public FrontendActionFactory
    {
    public:

        // create/destroy:
        optimizer_frontend_action_factory(FactoryT* factory, bobopt::optimizer* optimizer);
        virtual ~optimizer_frontend_action_factory() BOBOPT_OVERRIDE;

        // inherited overriden members:
        virtual FrontendAction* create() BOBOPT_OVERRIDE;

    private:

        /// \brief Wrapper for ASTFrontendAction that will actually catch instance of \c clang::CompilerInstance
        /// and pass this to optimizer object.
        class optimizer_frontend_action : public ASTFrontendAction
        {
        public:

            // create/destroy:
            optimizer_frontend_action(FactoryT* factory, bobopt::optimizer* optimizer);
            virtual ~optimizer_frontend_action() BOBOPT_OVERRIDE;

            // inherited overriden members:
            virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler_instance, StringRef) BOBOPT_OVERRIDE;

        private:
            FactoryT* factory_;
            bobopt::optimizer* optimizer_;
        };

        FactoryT* factory_;
        bobopt::optimizer* optimizer_;
    };

    // optimizer_frontend_action implementation.
    //==============================================================================

    /// \brief Create frontend action with all needed addition information.
    template <typename FactoryT>
    optimizer_frontend_action_factory<FactoryT>::optimizer_frontend_action::optimizer_frontend_action(FactoryT* factory, bobopt::optimizer* optimizer)
        : factory_(factory)
        , optimizer_(optimizer)
    {
    }

    /// \brief Deletable through pointer to base.
    template <typename FactoryT>
    optimizer_frontend_action_factory<FactoryT>::optimizer_frontend_action::~optimizer_frontend_action()
    {
    }

    /// \brief Immediately pass pointer to \c clang::CompilerInstance to optimizer object and create consumer using factory object.
    template <typename FactoryT>
    std::unique_ptr<ASTConsumer> optimizer_frontend_action_factory<FactoryT>::optimizer_frontend_action::CreateASTConsumer(clang::CompilerInstance& compiler_instance,
                                                                                                           StringRef)
    {
        optimizer_->set_compiler(&compiler_instance);
        return factory_->newASTConsumer();
    }

    // optimizer_frontend_action_factory implementation.
    //==============================================================================

    /// \brief Create factory with all needed additional information.
    template <typename FactoryT>
    optimizer_frontend_action_factory<FactoryT>::optimizer_frontend_action_factory(FactoryT* factory, bobopt::optimizer* optimizer)
        : factory_(factory)
        , optimizer_(optimizer)
    {
        BOBOPT_ASSERT(factory != nullptr);
        BOBOPT_ASSERT(optimizer != nullptr);
    }

    /// \brief Deletable through pointer to base.
    template <typename FactoryT>
    optimizer_frontend_action_factory<FactoryT>::~optimizer_frontend_action_factory()
    {
    }

    /// \brief Create wrapper of frontend action to catch \c clang::CompilerInstance.
    template <typename FactoryT>
    FrontendAction* optimizer_frontend_action_factory<FactoryT>::create()
    {
        return new optimizer_frontend_action(factory_, optimizer_);
    }

} // namespace

/// \brief Setting up configuration file from command line.
static llvm::cl::opt<std::string> opt_config_file("c", llvm::cl::desc("Specify config filename."), llvm::cl::value_desc("config file"));
/// \brief Generation of default configuration file.
static llvm::cl::opt<std::string> opt_gen_config_file("g", llvm::cl::desc("Generate default config file."), llvm::cl::value_desc("config file"));

/// \brief Command line option for program mode.
static llvm::cl::opt<bobopt::modes>
opt_mode(llvm::cl::desc("Optimizer mode:"),
         llvm::cl::initializer<bobopt::modes>(bobopt::MODE_DIAGNOSTIC),
         llvm::cl::values(clEnumValN(bobopt::MODE_DIAGNOSTIC, "diagnostic", "Print diagnostic. No modifications."),
                          clEnumValN(bobopt::MODE_INTERACTIVE, "interactive", "Modify code according to user input."),
                          clEnumValN(bobopt::MODE_BUILD, "build", "Do not print any diagnostic, just modify code."),
                          clEnumValEnd));

int main(int argc, const char* argv[])
{
    // Simple parsing only '-g' parameter as CommonOptionsParser needs also
    // at least one position argument as source file and of course its compilation
    // database.
    if ((argc == 3) && (std::string("-g") == argv[1]))
    {
        const std::string file_name = argv[2];

        bobopt::config_parser parser;
        if (!parser.save(file_name))
        {
            llvm::errs() << "Failed to save default configuration file to: " << file_name << '\n';
            return 1;
        }

        return 0;
    }

    llvm::cl::OptionCategory category("Tooling options");
    CommonOptionsParser options(argc, argv, category);

    if (opt_gen_config_file.getNumOccurrences() > 0)
    {
        const std::string file_name = opt_gen_config_file.c_str();

        bobopt::config_parser parser;
        if (!parser.save(file_name))
        {
            llvm::errs() << "Failed to save default configuration file to: " << file_name << '\n';
            return 1;
        }

        return 0;
    }

    if (opt_config_file.getNumOccurrences() > 0)
    {
        std::string file_name = opt_config_file.c_str();

        bobopt::config_parser parser;
        if (!parser.load(file_name))
        {
            llvm::errs() << "Failed to load configuration file: " << file_name << "... using defaults.\n";
        }
    }

    RefactoringTool tool(options.getCompilations(), options.getSourcePathList());

    bobopt::optimizer optimizer(opt_mode, &tool.getReplacements());

    MatchFinder finder;
    finder.addMatcher(bobopt::optimizer::BOBOX_BOX_MATCHER, &optimizer);
    finder.addMatcher(bobopt::optimizer::BOBOX_BASIC_BOX_MATCHER, &optimizer);
    finder.addMatcher(bobopt::optimizer::USER_BOX_MATCHER, &optimizer);

    bobopt::optimizer_frontend_action_factory<MatchFinder> frontend_action_factory(&finder, &optimizer);
    int result = tool.runAndSave(&frontend_action_factory);

    return result;
}
