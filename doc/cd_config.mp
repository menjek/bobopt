input metauml;
beginfig(1);

Interface.BasicVariable("basic_config_variable")
	("+get_name() : string {query}",
	 "+set(string value) : void",
	 "+default_value() : string {query}");

Class.Variable("config_variable")
	("-name : string",
	 "-value : T",
	 "-default_value : T",
	 "-parser : ParserT")
	("+get() : T {query}");
ClassTemplate.T("T", "ParserT")(Variable);

Class.ConfigGroup("config_group")
	("-name : string",
	 "-map : map<string, basic_config_variable>")
	("+get_variable(string) : basic_config_variable {query}",
	 "+variables_begin() : variable_iterator {query}",
 	 "+variables_end() : variable_iterator {query}");
 	
Class.Map("config_map")
	("-map : map<string, config_group>")
	("+get_group(string) : config_group {query}",
	 "+groups_begin() : group_iterator {query}",
 	 "+groups_end() : group_iterator {query}");

topToBottom(30)(BasicVariable, Variable);
leftToRight(30)(BasicVariable, ConfigGroup);
topToBottom(30)(ConfigGroup, Map);

drawObjects(Variable, T);
drawObjects(BasicVariable);
drawObjects(ConfigGroup);
drawObjects(Map);

link(aggregation)(BasicVariable.e -- ConfigGroup.w);
link(aggregation)(ConfigGroup.s -- Map.n);
link(inheritance)(Variable.n -- BasicVariable.s);

endfig;
end