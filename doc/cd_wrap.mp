input metauml;
beginfig(1);

Class.FrontendActionFactory("FrontendActionFactory")
	()
	("+create() : FrontendAction");

Class.FrontendAction("FrontendAction")
	()
	("+CreateASTConsumer(CompilerInstance, StringRef) : ASTConsumer");
	
Class.OptimizerFactory("optimizer_frontend_action_factory")
	("-factory : FactoryT", "-optimizer : optimizer")
	();
ClassTemplate.T("FactoryT")(OptimizerFactory);
	
Class.OptimizerAction("optimizer_frontend_action")
	("-factory : FactoryT", "-optimizer : optimizer")
	();
	
Note.A("This is private", "local class.");

Note.B("Place where", "CompilerInstance", "is caught");
	
topToBottom(30)(FrontendActionFactory, OptimizerFactory, OptimizerAction, FrontendAction);

A.e = OptimizerAction.w + (-30, 25);
B.sw = FrontendAction.ne + (-35, 25);

drawObjects(FrontendActionFactory, OptimizerFactory, OptimizerAction, FrontendAction, T, A, B);

link(inheritance)(OptimizerFactory.n -- FrontendActionFactory.s);
link(inheritance)(OptimizerAction.s -- FrontendAction.n);

clink(dashedLink)(A, OptimizerAction);
clink(dashedLink)(B, FrontendAction);
clink(dependency)(OptimizerFactory, OptimizerAction);
item(iStereo)("<<creates>>")(obj.w = .5[OptimizerAction.s,OptimizerFactory.n]);

endfig;
end