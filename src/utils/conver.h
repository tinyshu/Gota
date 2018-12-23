#ifndef CONVER_H__
#define CONVER_H__

#include <sstream>
template <class in_value,class out_type>
out_type convert(const in_value &t)
{
	out_type ans;
	std::stringstream a;
	a << t;
	a >> ans;
	return ans;
}

#endif