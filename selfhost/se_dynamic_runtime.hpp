#pragma once
// Type erasure retains the native shared-storage wrappers; boxing does not copy
// collection elements or user-object fields into a disconnected mirror.
template<class T> struct SECategory {static constexpr int kind=0;using element=void;};
template<class T> struct SECategory<SEList<T>> {static constexpr int kind=1;using element=T;};
template<class T> struct SECategory<SEMap<T>> {static constexpr int kind=2;using element=T;};
template<class T> struct SECategory<SESet<T>> {static constexpr int kind=3;using element=T;};

template<class T> struct SEDynamicModel final : SEDynamic {
  mutable T value;
  explicit SEDynamicModel(T item):value(std::move(item)){}
  std::string type()const override {
    if constexpr(SECategory<T>::kind==1)return "List";
    else if constexpr(SECategory<T>::kind==2)return "Map";
    else if constexpr(SECategory<T>::kind==3)return "Set";
    else if constexpr(requires {value.__se_type_name();})return value.__se_type_name();
    else return "Unknown";
  }
  std::string render()const override {
    if constexpr(SECategory<T>::kind!=0){std::ostringstream out;se_print_value(out,value);return out.str();}
    else return "<object>";
  }
  std::size_t size()const override {
    if constexpr(SECategory<T>::kind!=0)return value.size();
    else return SEDynamic::size();
  }
  SEAny index(const SEAny& key)const override {
    if constexpr(SECategory<T>::kind==1)return SEAny(static_cast<typename T::value_type>(se_index(value,se_any_cast<std::int64_t>(key))));
    else if constexpr(SECategory<T>::kind==2)return SEAny(value.at(se_any_cast<std::string>(key)));
    else return SEDynamic::index(key);
  }
  void assign(const SEAny& key,const SEAny& item)override {
    if constexpr(SECategory<T>::kind==1)se_assign_index(value,se_any_cast<std::int64_t>(key),se_any_cast<typename SECategory<T>::element>(item));
    else if constexpr(SECategory<T>::kind==2)value.put(se_any_cast<std::string>(key),se_any_cast<typename SECategory<T>::element>(item));
    else SEDynamic::assign(key,item);
  }
  void insert(const SEAny& key,const SEAny& item)override {
    if constexpr(SECategory<T>::kind==1){auto index=se_any_cast<std::int64_t>(key);if(index<0)index+=static_cast<std::int64_t>(value.size());index=std::clamp<std::int64_t>(index,0,static_cast<std::int64_t>(value.size()));auto converted=se_any_cast<typename SECategory<T>::element>(item);value.emplace_back();for(std::size_t i=value.size()-1;i>static_cast<std::size_t>(index);--i)value[i]=std::move(value[i-1]);value[static_cast<std::size_t>(index)]=std::move(converted);}
    else SEDynamic::insert(key,item);
  }
  SEAny pop(const SEAny& key)override {
    if constexpr(SECategory<T>::kind==1){if(value.empty())throw std::runtime_error("data.pop cannot pop an empty List.");auto index=se_any_cast<std::int64_t>(key);if(index<0)index+=static_cast<std::int64_t>(value.size());if(index<0||static_cast<std::uint64_t>(index)>=value.size())throw std::runtime_error("data.pop index is out of bounds.");auto pos=static_cast<std::size_t>(index);SEAny out(value[pos]);value.erase(value.begin()+static_cast<std::ptrdiff_t>(pos));return out;}
    else return SEDynamic::pop(key);
  }
  void clear()override {
    if constexpr(SECategory<T>::kind==1)value.clear();
    else if constexpr(SECategory<T>::kind==2 || SECategory<T>::kind==3)value.items.clear();
    else SEDynamic::clear();
  }
  SEAny shallow_copy()const override {
    if constexpr(SECategory<T>::kind==1 || SECategory<T>::kind==2 || SECategory<T>::kind==3)return SEAny(value.clone());
    else if constexpr(std::is_copy_constructible_v<T>)return SEAny(value);
    else return SEDynamic::shallow_copy();
  }
  void add(const SEAny& item)override {
    if constexpr(SECategory<T>::kind==1 && std::is_same_v<typename SECategory<T>::element,SEAny>)value.push_back(item);
    else if constexpr(SECategory<T>::kind==3 && std::is_same_v<typename SECategory<T>::element,SEAny>)value.add(item);
    else if constexpr(SECategory<T>::kind==1 || SECategory<T>::kind==3)se_add_item(value,se_any_cast<typename SECategory<T>::element>(item));
    else SEDynamic::add(item);
  }
  bool remove(const SEAny& item)override {
    if constexpr(SECategory<T>::kind==1 && std::is_same_v<typename SECategory<T>::element,SEAny>){auto pos=std::find(value.begin(),value.end(),item);if(pos==value.end())return false;value.erase(pos);return true;}
    else if constexpr(SECategory<T>::kind==3 && std::is_same_v<typename SECategory<T>::element,SEAny>){auto pos=std::find(value.items.begin(),value.items.end(),item);if(pos==value.items.end())return false;value.items.erase(pos);return true;}
    else if constexpr(SECategory<T>::kind==1 || SECategory<T>::kind==3)return se_remove_item(value,se_any_cast<typename SECategory<T>::element>(item));
    else if constexpr(SECategory<T>::kind==2)return se_remove_item(value,se_any_cast<std::string>(item));
    else return SEDynamic::remove(item);
  }
  SEAny member(const std::string& name)override {
    if constexpr(requires {value.__se_dynamic_get(name);})return value.__se_dynamic_get(name);
    else return SEDynamic::member(name);
  }
  void set_member(const std::string& name,const SEAny& item)override {
    if constexpr(requires {value.__se_dynamic_set(name,item);})value.__se_dynamic_set(name,item);
    else SEDynamic::set_member(name,item);
  }
  SEAny call(const std::string& name,const SEList<SEAny>& args)override {
    if constexpr(requires {value.__se_dynamic_call(name,args);})return value.__se_dynamic_call(name,args);
    else return SEDynamic::call(name,args);
  }
  SEList<SEAny> iterate()const override {
    SEList<SEAny> out;
    if constexpr(SECategory<T>::kind==1){for(const auto& item:value)out.emplace_back(static_cast<typename T::value_type>(item));}
    else if constexpr(SECategory<T>::kind==2){for(const auto& item:value.items)out.emplace_back(item.first);}
    else if constexpr(SECategory<T>::kind==3){for(const auto& item:value.items)out.emplace_back(item);}
    else return SEDynamic::iterate();
    return out;
  }
  SEList<std::pair<std::string,SEAny>> pairs()const override {
    if constexpr(SECategory<T>::kind==2){SEList<std::pair<std::string,SEAny>> out;for(const auto& item:value.items)out.emplace_back(item.first,SEAny(item.second));return out;}
    else return SEDynamic::pairs();
  }
  bool equal(const SEDynamic& other)const override {
    const auto* model=dynamic_cast<const SEDynamicModel*>(&other);
    if constexpr(requires {value==value;})return model && value==model->value;
    else return model==this;
  }
};
template<class T> requires (!std::is_constructible_v<SEAny::Base,T> && !std::is_same_v<std::decay_t<T>,SEAny>)
SEAny::SEAny(T value):Base(std::static_pointer_cast<SEDynamic>(std::make_shared<SEDynamicModel<std::decay_t<T>>>(std::move(value)))){}

template<class T> T se_any_cast(const SEAny& value){
  if constexpr(std::is_same_v<T,SEAny>)return value;
  else if constexpr(std::is_same_v<T,std::int64_t>){if(auto p=std::get_if<std::int64_t>(&value))return *p;}
  else if constexpr(std::is_same_v<T,double>){if(auto p=std::get_if<double>(&value))return *p;if(auto p=std::get_if<std::int64_t>(&value))return static_cast<double>(*p);}
  else if constexpr(std::is_same_v<T,bool> || std::is_same_v<T,std::string> || std::is_same_v<T,SEBytes>){if(auto p=std::get_if<T>(&value))return *p;}
  else if(auto p=std::get_if<std::shared_ptr<SEDynamic>>(&value)){
    if(auto model=std::dynamic_pointer_cast<SEDynamicModel<T>>(*p))return model->value;
  }
  throw std::runtime_error("Dynamic value has the wrong type.");
}
inline std::shared_ptr<SEDynamic> se_any_box(const SEAny& value){
  if(auto p=std::get_if<std::shared_ptr<SEDynamic>>(&value))return *p;
  throw std::runtime_error("This dynamic value is not a container or object.");
}
inline std::size_t SEAny::size()const{
  if(auto p=std::get_if<std::string>(this))return p->size();
  if(auto p=std::get_if<SEBytes>(this))return p->size();
  return se_any_box(*this)->size();
}
inline SEAny::operator bool()const{return se_any_cast<bool>(*this);}
inline bool se_bool(const SEAny& value){if(auto p=std::get_if<std::string>(&value))return se_bool(*p);return se_any_cast<bool>(value);}
inline SEAny se_index(const SEAny& value,const SEAny& key){
  if(auto p=std::get_if<SEBytes>(&value))return SEAny(se_index(*p,se_any_cast<std::int64_t>(key)));
  return se_any_box(value)->index(key);
}
inline void se_assign_index(SEAny& value,const SEAny& key,const SEAny& item){
  if(auto p=std::get_if<SEBytes>(&value)){se_assign_index(*p,se_any_cast<std::int64_t>(key),se_any_cast<std::int64_t>(item));return;}
  se_any_box(value)->assign(key,item);
}
inline void se_add_item(SEAny& value,const SEAny& item){se_any_box(value)->add(item);}
inline bool se_remove_item(SEAny& value,const SEAny& item){return se_any_box(value)->remove(item);}
inline SEAny se_any_member(SEAny value,const std::string& name){return se_any_box(value)->member(name);}
inline void se_any_set_member(SEAny value,const std::string& name,const SEAny& field){se_any_box(value)->set_member(name,field);}
inline SEAny se_any_call(SEAny value,const std::string& name,const SEList<SEAny>& args){return se_any_box(value)->call(name,args);}
template<class F> SEAny se_box_call(F&& action){if constexpr(std::is_void_v<std::invoke_result_t<F>>) {action();return {};}else return SEAny(action());}

inline bool se_any_numeric(const SEAny& v){return std::holds_alternative<std::int64_t>(v)||std::holds_alternative<double>(v);}
inline double se_any_number(const SEAny& v){if(!se_any_numeric(v))throw std::runtime_error("Arithmetic requires numbers.");return se_num(v);}
inline bool operator==(const SEAny& a,const SEAny& b){
  if(auto x=std::get_if<std::int64_t>(&a)){if(auto y=std::get_if<std::int64_t>(&b))return *x==*y;}
  if(se_any_numeric(a)&&se_any_numeric(b))return se_num(a)==se_num(b);
  if(a.index()!=b.index())return false;
  if(auto x=std::get_if<std::shared_ptr<SEDynamic>>(&a))return (*x)->equal(*std::get<std::shared_ptr<SEDynamic>>(b));
  return static_cast<const SEAny::Base&>(a)==static_cast<const SEAny::Base&>(b);
}
inline SEAny operator+(const SEAny& a,const SEAny& b){
  if(std::holds_alternative<std::string>(a)||std::holds_alternative<std::string>(b))return se_string(a)+se_string(b);
  if(auto x=std::get_if<std::int64_t>(&a))if(auto y=std::get_if<std::int64_t>(&b))return *x+*y;
  return se_any_number(a)+se_any_number(b);
}
inline SEAny operator-(const SEAny& a,const SEAny& b){
  if(auto x=std::get_if<std::int64_t>(&a))if(auto y=std::get_if<std::int64_t>(&b))return *x-*y;
  return se_any_number(a)-se_any_number(b);
}
inline SEAny operator*(const SEAny& a,const SEAny& b){
  if(auto x=std::get_if<std::int64_t>(&a))if(auto y=std::get_if<std::int64_t>(&b))return *x**y;
  return se_any_number(a)*se_any_number(b);
}
inline SEAny operator%(const SEAny& a,const SEAny& b){
  if(auto x=std::get_if<std::int64_t>(&a))if(auto y=std::get_if<std::int64_t>(&b))return se_remainder(*x,*y);
  return se_remainder(se_any_number(a),se_any_number(b));
}
inline SEAny operator-(const SEAny& a){if(auto x=std::get_if<std::int64_t>(&a))return -*x;return -se_any_number(a);}
inline bool operator<(const SEAny& a,const SEAny& b){return se_any_number(a)<se_any_number(b);}
inline bool operator<=(const SEAny& a,const SEAny& b){return se_any_number(a)<=se_any_number(b);}
inline bool operator>(const SEAny& a,const SEAny& b){return se_any_number(a)>se_any_number(b);}
inline bool operator>=(const SEAny& a,const SEAny& b){return se_any_number(a)>=se_any_number(b);}

template<class T> SEAny se_box_value(const T& value){if constexpr(std::is_copy_constructible_v<T>)return SEAny(value);else throw std::runtime_error("This dynamic value cannot be copied.");}

template<class T> auto se_iterate(T value){return value;}
inline SEList<SEAny> se_iterate(const SEAny& value){return se_any_box(value)->iterate();}
template<class T> auto se_pairs(const SEMap<T>& value){return value.items;}
inline auto se_pairs(const SEAny& value){return se_any_box(value)->pairs();}
inline bool se_contains(const SEAny& values,const SEAny& item){
  if(auto text=std::get_if<std::string>(&values))return text->find(se_any_cast<std::string>(item))!=std::string::npos;
  for(const auto& value:se_any_box(values)->iterate())if(value==item)return true;
  return false;
}
template<class T> requires std::is_same_v<std::decay_t<T>,SEAny> inline std::string se_upper(const T& value){return se_upper(se_any_cast<std::string>(value));}
template<class T> requires std::is_same_v<std::decay_t<T>,SEAny> inline std::string se_lower(const T& value){return se_lower(se_any_cast<std::string>(value));}
template<class A,class B> auto se_power(const A& a,const B& b){if constexpr(std::is_same_v<std::decay_t<A>,SEAny> || std::is_same_v<std::decay_t<B>,SEAny>)return std::pow(se_any_number(SEAny(a)),se_any_number(SEAny(b)));else return std::pow(a,b);}
inline double se_power(const SEAny& a,const SEAny& b){return std::pow(se_any_number(a),se_any_number(b));}

template<class A,class B> double se_divide(const A& a,const B& b){
  const double left=[&]{if constexpr(std::is_same_v<std::decay_t<A>,SEAny>)return se_any_number(a);else return static_cast<double>(a);}();
  const double right=[&]{if constexpr(std::is_same_v<std::decay_t<B>,SEAny>)return se_any_number(b);else return static_cast<double>(b);}();
  if(right==0)throw std::runtime_error("Division by zero.");
  return left/right;
}
template<class U> void se_add_item(SEList<SEAny>& values,const U& value){values.push_back(SEAny(value));}
template<class U> void se_add_item(SESet<SEAny>& values,const U& value){values.add(SEAny(value));}
template<class U> bool se_remove_item(SEList<SEAny>& values,const U& value){
  const SEAny item(value);const auto pos=std::find(values.begin(),values.end(),item);
  if(pos==values.end())return false;values.erase(pos);return true;
}
template<class U> bool se_remove_item(SESet<SEAny>& values,const U& value){
  const SEAny item(value);const auto pos=std::find(values.items.begin(),values.items.end(),item);
  if(pos==values.items.end())return false;values.items.erase(pos);return true;
}
