#pragma once

#include <vector>
#include <iostream>
#include <fstream>

namespace Tools
	{
	class Serializer
		{
		private:
			std::fstream& fs;
			bool reading;

		public:
			Serializer( std::fstream& _fs , bool _reading ) : fs( _fs ), reading( _reading ) {}

			template<class type> void Item( type& item )
				{
				if( this->reading )
					this->fs.read( (char*)&item, sizeof( type ) );
				else
					this->fs.write( (char*)&item, sizeof( type ) );
				}

			template<class vectortype> void VectorSize( vectortype& _vector )
				{
				if( this->reading )
					{
					// read count from stream and resize vector
					size_t count;
					this->Item( count ); 
					_vector.resize( count ); 
					}
				else
					{
					// write vector size to stream
					size_t count = _vector.size();
					this->Item( count );
					}
				}

			template<class vectortype> void Vector( vectortype& _vector )
				{
				this->VectorSize( _vector ); // first serialize the length of the vector
				if( this->reading )
					this->fs.read( (char*)_vector.data(), sizeof( vectortype::value_type ) * _vector.size() );
				else
					this->fs.write( (char*)_vector.data(), sizeof( vectortype::value_type ) * _vector.size() );
				}
		};
	};
