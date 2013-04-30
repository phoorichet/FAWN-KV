#pragma once

#include "common.hpp"
#include <iostream>

#include <tr1/cstdint>
#include <boost/numeric/conversion/converter.hpp>

using namespace std;
namespace cindex
{
	// guarded static_cast
	template<typename T, typename S>
	inline T guarded_cast(const S& v)
	{
    cout << "converting " << v << "->" << static_cast<T>(v) << endl;
// #ifndef NDEBUG
// //     T ret = boost::numeric::converter<T, S>::convert(v); 
// //     cout<< "++++++++++++0000+++++++++++++++"<< ret <<endl;
// 		return boost::numeric::converter<T, S>::convert(v); 
// #else
		return static_cast<T>(v);
// #endif

    
	}
}

