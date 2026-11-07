#pragma once

inline std::vector<SEAny> se_model_items(const SEAny& value) {
  auto boxed=std::get_if<std::shared_ptr<SEDynamic>>(&value);
  if(!boxed)throw std::runtime_error("Expected a List.");
  if((*boxed)->type()!="List")throw std::runtime_error("Expected a List.");
  auto items=(*boxed)->iterate();
  return std::vector<SEAny>(items.begin(),items.end());
}
inline std::vector<std::pair<std::string,SEAny>> se_model_fields(const SEAny& value) {
  auto boxed=std::get_if<std::shared_ptr<SEDynamic>>(&value);
  if(!boxed)throw std::runtime_error("Expected a Map.");
  if((*boxed)->type()!="Map")throw std::runtime_error("Expected a Map.");
  auto fields=(*boxed)->pairs();
  return std::vector<std::pair<std::string,SEAny>>(fields.begin(),fields.end());
}
inline SEAny se_model_index(const SEAny& value,const std::string& key) {
  return se_index(value,SEAny(key));
}
inline SEAny se_dataset_describe(const SEAny& rows) {
  auto values=se_model_items(rows); SEList<SEAny> columns;
  if(!values.empty())for(const auto& field:se_model_fields(values.front()))columns.emplace_back(field.first);
  SEMap<SEAny> result;result.put("row_count",SEAny(static_cast<std::int64_t>(values.size())));result.put("columns",SEAny(columns));return SEAny(result);
}
inline SEAny se_dataset_train_test_split(const SEAny& rows,double ratio) {
  if(!(ratio>0&&ratio<1))throw std::runtime_error("Split ratio must be between 0 and 1.");
  auto values=se_model_items(rows);auto cut=static_cast<std::size_t>(values.size()*ratio);
  SEList<SEAny> first(values.begin(),values.begin()+static_cast<std::ptrdiff_t>(cut));
  SEList<SEAny> second(values.begin()+static_cast<std::ptrdiff_t>(cut),values.end());
  return SEAny(SEList<SEAny>{SEAny(first),SEAny(second)});
}
inline SEAny se_dataset_filter_eq(const SEAny& rows,const std::string& column,const SEAny& target) {
  SEList<SEAny> out;
  for(const auto& row:se_model_items(rows)){
    auto fields=se_model_fields(row);auto it=std::find_if(fields.begin(),fields.end(),[&](const auto& item){return item.first==column;});
    if(it==fields.end())throw std::runtime_error("Missing dataset column: "+column);
    if(it->second==target)out.push_back(row);
  }
  return SEAny(out);
}
inline SEAny se_dataset_unique(const SEAny& rows,const std::string& column) {
  SEList<SEAny> out,seen;
  for(const auto& row:se_model_items(rows)){
    auto fields=se_model_fields(row);auto it=std::find_if(fields.begin(),fields.end(),[&](const auto& item){return item.first==column;});
    if(it==fields.end())throw std::runtime_error("Missing dataset column: "+column);
    if(!se_contains(seen,it->second)){seen.push_back(it->second);out.push_back(row);}
  }
  return SEAny(out);
}
inline SEAny se_ml_linear_regression(const SEAny& xs,const SEAny& ys) {
  auto x=se_model_items(xs),y=se_model_items(ys);
  if(x.size()!=y.size()||x.size()<2)throw std::runtime_error("Regression needs matching numeric Lists of length >= 2.");
  double mx=0,my=0;for(std::size_t i=0;i<x.size();++i){mx+=se_num(x[i]);my+=se_num(y[i]);}mx/=x.size();my/=y.size();
  double denominator=0,numerator=0;for(std::size_t i=0;i<x.size();++i){auto dx=se_num(x[i])-mx;denominator+=dx*dx;numerator+=dx*(se_num(y[i])-my);}
  if(denominator==0)throw std::runtime_error("All x values are equal.");
  double slope=numerator/denominator;SEMap<SEAny> model;model.put("slope",SEAny(slope));model.put("intercept",SEAny(my-slope*mx));return SEAny(model);
}
inline double se_ml_predict(const SEAny& model,double x) {
  return se_num(se_model_index(model,"slope"))*x+se_num(se_model_index(model,"intercept"));
}
inline SEList<std::int64_t> se_tensor_shape_inner(const SEAny& value) {
  auto boxed=std::get_if<std::shared_ptr<SEDynamic>>(&value);
  if(!boxed){if(se_any_numeric(value))return {};throw std::runtime_error("Tensor values must be numeric.");}
  if((*boxed)->type()!="List")throw std::runtime_error("Tensor values must be Lists or numbers.");
  auto items=se_model_items(value);if(items.empty())return {0};auto inner=se_tensor_shape_inner(items.front());
  for(std::size_t i=1;i<items.size();++i)if(se_tensor_shape_inner(items[i])!=inner)throw std::runtime_error("Tensor must be rectangular.");
  SEList<std::int64_t> result{static_cast<std::int64_t>(items.size())};for(auto dimension:inner)result.push_back(dimension);return result;
}
inline SEAny se_tensor_shape(const SEAny& value){return SEAny(se_tensor_shape_inner(value));}
inline SEAny se_tensor_add_inner(const SEAny& left,const SEAny& right) {
  auto a=std::get_if<std::shared_ptr<SEDynamic>>(&left),b=std::get_if<std::shared_ptr<SEDynamic>>(&right);
  if(a||b){if(!a||!b||(*a)->type()!="List"||(*b)->type()!="List")throw std::runtime_error("Tensor dimensions do not match.");auto x=se_model_items(left),y=se_model_items(right);if(x.size()!=y.size())throw std::runtime_error("Tensor dimensions do not match.");SEList<SEAny> out;for(std::size_t i=0;i<x.size();++i)out.push_back(se_tensor_add_inner(x[i],y[i]));return SEAny(out);}
  return SEAny(se_num(left)+se_num(right));
}
inline SEAny se_tensor_add(const SEAny& left,const SEAny& right){if(se_tensor_shape_inner(left)!=se_tensor_shape_inner(right))throw std::runtime_error("Tensor dimensions do not match.");return se_tensor_add_inner(left,right);}
inline SEAny se_tensor_matmul(const SEAny& left,const SEAny& right) {
  auto a_shape=se_tensor_shape_inner(left),b_shape=se_tensor_shape_inner(right);
  if(a_shape.size()!=2||b_shape.size()!=2||a_shape[1]!=b_shape[0])throw std::runtime_error("matmul needs compatible 2D tensors.");
  auto a=se_model_items(left),b=se_model_items(right);SEList<SEAny> result;
  for(const auto& row_value:a){auto row=se_model_items(row_value);SEList<SEAny> out_row;
    for(std::int64_t col=0;col<b_shape[1];++col){double sum=0;for(std::int64_t k=0;k<a_shape[1];++k)sum+=se_num(row[static_cast<std::size_t>(k)])*se_num(se_model_items(b[static_cast<std::size_t>(k)])[static_cast<std::size_t>(col)]);out_row.emplace_back(sum);}result.emplace_back(out_row);
  }
  return SEAny(result);
}

inline SEMap<SEAny> se_args_parse(const SEList<std::string>& args) {
  SEMap<SEAny> result;SEList<SEAny> positional;
  for(std::size_t i=0;i<args.size();++i){auto token=args[i];if(token.rfind("--",0)!=0){positional.emplace_back(token);continue;}
    token.erase(0,2);auto equals=token.find('=');if(equals!=std::string::npos){result.put(token.substr(0,equals),SEAny(token.substr(equals+1)));continue;}
    if(i+1<args.size()&&args[i+1].rfind("--",0)!=0){result.put(token,SEAny(args[++i]));continue;}result.put(token,SEAny(true));
  }
  if(!positional.empty())result.put("_",SEAny(positional));return result;
}
inline SEAny se_args_get(const SEMap<SEAny>& args,const std::string& name){return args.at(name);}
inline SEAny se_args_get(const SEMap<SEAny>& args,const std::string& name,const SEAny& fallback){for(const auto& item:args.items)if(item.first==name)return item.second;return fallback;}
inline bool se_args_flag(const SEMap<SEAny>& args,const std::string& name){for(const auto& item:args.items)if(item.first==name){if(auto value=std::get_if<bool>(&item.second))return *value;if(auto value=std::get_if<std::string>(&item.second))return se_bool(*value);return true;}return false;}
inline SEMap<SEAny> se_argparse_parse_args(const SEList<std::string>& args){return se_args_parse(args);}
inline bool se_argparse_has(const SEMap<SEAny>& args,const std::string& name){for(const auto& item:args.items)if(item.first==name)return true;return false;}
inline SEAny se_argparse_positionals(const SEMap<SEAny>& args){return se_args_get(args,"_",SEAny(SEList<SEAny>{}));}
inline SEAny se_argparse_require(const SEMap<SEAny>& args,const std::string& name){if(!se_argparse_has(args,name))throw std::runtime_error("Required command-line option is missing: "+name);return args.at(name);}
inline std::int64_t se_argparse_get_int(const SEMap<SEAny>& args,const std::string& name){auto value=se_argparse_require(args,name);try{std::size_t used=0;auto raw=se_string(value);auto number=std::stoll(raw,&used);if(used!=raw.size())throw std::runtime_error("bad");return number;}catch(...){throw std::runtime_error("Option is not an Int: "+name);}}
inline std::string se_argparse_help(const std::string& title,const SEList<std::string>& options){std::ostringstream out;out<<title<<"\nOptions:\n";for(const auto& option:options)out<<"  --"<<option<<"\n";return out.str();}
