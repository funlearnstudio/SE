#include "se_bootstrap_runtime.hpp"
#include <cassert>
int main(){
 const std::string metadata="native/native_test.snative", library="native_test";
 const auto empty=se_native_invoke<SENativeBytesView>(metadata,library,"s_test_bytes_echo",false,"","","bytes_echo",SENativeBytesView{nullptr,0});
 assert(empty.empty());
 SEBytes bytes{0,1,127,255};
 auto copied=se_native_invoke<SENativeBytesView>(metadata,library,"s_test_bytes_echo",false,"","","bytes_echo",se_native_argument(bytes));
 assert(copied==bytes && copied.data()!=bytes.data());
 auto bytes_alias=copied;bytes_alias[0]=9;assert(copied[0]==9);
 bool range_error=false;try{(void)se_native_argument(std::int64_t{1}<<40);}catch(const SENativeError&){range_error=true;}assert(range_error);
 auto live=[&]{return se_native_invoke<int>(metadata,library,"s_test_live_counters",false,"","","live_counters");};
 assert(live()==0);
 {
 auto handle=se_native_invoke<void*>(metadata,library,"s_test_counter_create",false,"s_test_counter_destroy","Counter","make_counter");
 assert(live()==1);
 auto copy=handle;handle.resource.reset();assert(live()==1);
 bool caught=false;try{se_native_handle(copy,"Other");}catch(const SENativeError&){caught=true;}assert(caught);
 }
 assert(live()==0);
 bool caught=false;
 try{se_native_invoke<void*>(metadata,library,"s_test_counter_create",false,"missing_cleanup","Counter","make_counter");}catch(const SENativeError&){caught=true;}
 assert(caught&&live()==0);
 std::cout<<"Standalone native ABI lifetime and Bytes tests passed\n";
}
