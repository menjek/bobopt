input metauml;
beginfig(1);

Interface.BasicMethod("basic_method")
	("+optimize(CXXRecordDecl, Replacements)",
	 "#get_optimizer() : optimizer")
	("optimizer : optimizer");
	
Class.Prefetch("prefetch")
	()
	();
	
Class.YieldComplex("yield_complex")
	()
	();

Class.Optimizer("optimizer")
	()
	("+get_mode() : modes",
	 "+verbose() : bool",
	 "+get_diagnostic() : diagnostic",
	 "+get_compiler() : CompilerInstance");

leftToRight(25)(BasicMethod, Optimizer);

YieldComplex.width = Prefetch.width = 75;

Prefetch.n = BasicMethod.s + (-50, -50);
YieldComplex.n = BasicMethod.s + (50, -50);

drawObjects(BasicMethod, Prefetch, YieldComplex, Optimizer);

link(inheritance)(Prefetch.n -- Prefetch.n + (0, 25) -- BasicMethod.s + (0, -25) --  BasicMethod.s);
link(inheritance)(YieldComplex.n -- YieldComplex.n + (0,25) -- BasicMethod.s + (0, -25) -- BasicMethod.s);

link(aggregation)(BasicMethod.e -- Optimizer.w);

endfig;
end