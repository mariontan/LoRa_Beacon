// Clear an array buffer
template<typename T> void clear_buf(T* buf) {
  // Clear buffer content by filling it with NULL
  #ifdef DEBUG_BUFFER
  debug_log("Function", "Clear Array Buffer");
  #endif
  uint8_t* for_clear = (uint8_t*) buf;
  for (int i = 0; i < sizeof(for_clear); i++) { for_clear[i] = 0; }
}

/***getting the data type of variable***/
//*// Generic catch-all implementation. 
template <typename T_ty> struct TypeInfo { static const char * name; };
template <typename T_ty> const char * TypeInfo<T_ty>::name = "unknown";

// Handay macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name

// Handay macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;

/*
// Type-specific implementations.
MAKE_TYPE_INFO( int )
MAKE_TYPE_INFO( float )
MAKE_TYPE_INFO( short )
MAKE_TYPE_INFO( uint8_t )//*/
