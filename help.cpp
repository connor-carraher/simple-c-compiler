Expression

	virtual Expression *getDereference() const{ return nullptr;}

Dereference
	
	virtual Expression *getDereference() const { return _expr }


Lecture #21
	
	Adding spills

	if(reg -> _node != expr) {
		if(reg -> _node != nullptr){
			size = reg->_node->type().size();
			assigntemp(...);
		}
	}


In Call Generate:

	for( i = 0; i < args.size(), i++)
		_args(i)->generate();

	for(i = 0; i < registers.size(); i++)
		load(nullptr, registers[i]);
	//do this before loading arguments