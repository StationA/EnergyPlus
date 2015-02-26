// copyright 2014, Shannon Mackey <mackey@BUILDlab.net>
#include "JSONDataInterface.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <type_traits>

//#include "idd-minimal-ordered.h"

//forward declarations
void randomize();
std::string getUuid();


using namespace std;
using namespace idfx;

IDDxField::IDDxField(string name):
    _field_name(name),
    _field_properties(new map<string, string>())
{

}

IDDxField::~IDDxField()
{
    delete _field_properties;
}

void IDDxField::insertFieldProperties(string key, string value)
{
    _field_properties->emplace(key, value);
}

string IDDxField::value(string property_type)
{
    auto find_property(_field_properties->find(property_type));
    return (find_property != _field_properties->end())
           ? find_property->second
           : "";
}

uint32_t IDDxField::iddIndex()
{
    auto find_property(_field_properties->find("idd_position"));
    return (find_property != _field_properties->end())
           ? stoi(find_property->second)
           : 0; // fields are 1-based
}


IDDxObject::IDDxObject(cJSON *obj) :
    _object_type(""),
    _fields(new map<string, IDDxField>),
    _object_properties(new map<string, string>)
{
    if (obj) {
        _object_type = obj->string;
        cout << "\n\n #### " << _object_type << endl;
        cJSON *fld = obj->child ;
        if (fld) {
            while (fld) { //object property
                string sub_obj = fld->string;
                cout << " -- " << sub_obj << endl;
                if ((sub_obj == "extension_type")
                        || (sub_obj == "group")
                        || (sub_obj == "memo")
                        || (sub_obj == "unique_object")
                        || (sub_obj == "required_object")
                        || (sub_obj == "extension")) {

                    cout << sub_obj << " == " << fld->valuestring << endl;
                    _object_properties->emplace(sub_obj, fld->valuestring);

                } else if (sub_obj == "reference") {
                    cout << "reference : array" << endl;
                } else { //object field
                    IDDxField *field = new IDDxField(sub_obj);
                    cJSON *prop = fld->child;
                    if (prop) {
                        while (prop) {

                            switch (prop->type) {
                            case cJSON_Number: { //get numeric types
                                string key = prop->string;
                                double property_value((prop->valuedouble) ? prop->valuedouble : prop->valueint);
                                cout << key << " : " << to_string(property_value) << endl;
                                field->insertFieldProperties(key, to_string(property_value));
                                break;
                            }
                            case cJSON_String: { //get alpha types
                                string key = prop->string;
                                string property_value(prop->valuestring);
                                cout << key << " : " << property_value << endl;
                                field->insertFieldProperties(key, property_value);
                                break;
                            }
                            case cJSON_Array: { //TODO: object_list and reference always get treated like array - fix in input iddx
                                string key = prop->string;
//                             string property_value = prop->valuestring;
                                cout << key << " : " << "array" << endl;
//                             iterate array
//                             field->insertFieldProperties(key, property_value);
                                field->insertFieldProperties(key, "array");
                                break;
                            }
                            default: {
                                // nothing to do
                                break;
                            }
                            }
                            prop = prop->next;
                        }
                    }
                }
                fld = fld->next;
            }
        } else {
            cout << "\nERROR: schema object didn't import correctly : " << _object_type  << endl;
        }
    } else {
        cout << "\nERROR: invalid object in schema" << endl;
    }
}

IDDxObject::~IDDxObject()
{
    delete _fields;
    delete _object_properties;
}

bool IDDxObject::isValid()
{
    return false; //TODO: fix me,use me - (_fields->size() && (type() != "")) ? true : false;
}


void IDDxObject::insertField(IDDxField iddx_field)
{
    _fields->emplace(iddx_field.fieldName(), iddx_field);
}

string IDDxObject::fieldValue(string field_name, string property_type)
{
    auto find_field(_fields->find(field_name));
    if (find_field != _fields->end()) {
        auto found_field = find_field->second;
        return found_field.value(property_type);
    } else return "";
}

string IDDxObject::propertyValue(string property_name)
{
    auto find_property(_object_properties->find(property_name));
    if (find_property != _object_properties->end()) {
        return find_property->second;
    } else return "";
}

string IDDxObject::idd_field(uint32_t field_index)
{
    cout << " idd_field" << endl;
    auto flds =  orderedFieldNames();

//for (auto const & one_field : *_fields) {

//         auto one_field_properties = one_field.second;
//         if (one_field_properties->iddIndex() == field_index) {
//             return one_field.first;
//         }
    //  }
}

vector< string > IDDxObject::orderedFieldNames()
{
    cout << " orderedFieldNames" << endl;

    uint32_t field_count = _fields->size();
    vector<string>  return_vector(field_count);

//TODO: do this

}


IDDxObjects::IDDxObjects(const string &json_content) :
    _iddx_object_map(new map<string, IDDxObject * >())
{
    cJSON *data_dictionary(cJSON_Parse(json_content.c_str()));
    if (data_dictionary) {
        if (!loadIDDxObjects(data_dictionary))
            cout << "\nERROR: failed to populate schema object map." << endl;
        cJSON_Delete(data_dictionary);
    } else {
        cout << "\nERROR: json schema string load failure - \n" << endl << cJSON_GetErrorPtr();
    }
}

IDDxObjects::~IDDxObjects()
{
    delete _iddx_object_map;
}


bool IDDxObjects::loadIDDxObjects(cJSON *schema_root)
{
    cJSON *obj(schema_root->child);
    while (obj) {
        auto one_object = new IDDxObject(obj);
//             if (one_object.isValid()) {
        insertIDDxObject(one_object);
        /*} else {
            cout << "FAIL: " << one_object.type() << endl;
            return false;
        }*/
        obj = obj->next;
    }
    return true;
}

void IDDxObjects::insertIDDxObject(IDDxObject *iddx_object)
{
    _iddx_object_map->emplace(iddx_object->objectType(), iddx_object);
}

string IDDxObjects::getIDDxObjectFieldPropertyValue(string iddx_object_type, string field_name, string property_type) const
{
    auto find_object(_iddx_object_map->find(iddx_object_type));
    if (find_object != _iddx_object_map->end()) {
        auto found_object = find_object->second;
        return found_object->fieldValue(field_name, property_type);
    } else return "";
}

string IDDxObjects::getIDDxObjectPropertyValue(string iddx_object_type, string property_type) const
{
    auto find_object(_iddx_object_map->find(iddx_object_type));
    if (find_object != _iddx_object_map->end()) {
        auto found_object = find_object->second;
        return found_object->propertyValue(property_type);
    } else return "";
}

IDDxObject *IDDxObjects::getIDDxObject(const string iddx_object_type) const
{
    auto find_object(_iddx_object_map->find(iddx_object_type));
    if (find_object != _iddx_object_map->end()) {
        return find_object->second;
    } else {
        return nullptr;
    }
}


IDFxObject::IDFxObject(const string &json_content, const IDDxObjects &schema_objects) :
    _id(""),
    _object_type(""),
    _schema_object(nullptr),
    _properties(new std::map<std::string, std::string>()),
    _extensions(new std::vector<IDFxObject *>)
{
    cJSON *valid_object(cJSON_Parse(json_content.c_str()));
    if (valid_object) {
        setProperties(valid_object);
        _id = value("_id");
        _object_type = value("_type");
        _schema_object = schema_objects.getIDDxObject(_object_type);
        if (_schema_object) {
            string extension_type(_schema_object->propertyValue("extension_type"));
            if (extension_type != "") {
                //TODO: include an extensible object in sample input file to test
                setExtensions(valid_object, extension_type, schema_objects);
            }
        } else {
            cout << "\nERROR: failed to get _schema_object for: " << _object_type << endl;
        }
    } else {
        cout << "\nERROR: failed to extract object type from cJSON structure." << endl;
    }
}


IDFxObject::~IDFxObject()
{
    delete _extensions;
    delete _properties;
}

void IDFxObject::setExtensions(cJSON *cjson_object, string extension_type, const IDDxObjects schema_objects)
{
    if (cjson_object) {
        cJSON *json_child(cjson_object->child);
        if (json_child) {

//TODO: implement this when including sample extension objects

            do {
                _extensions->emplace_back(new IDFxObject(cJSON_Print(json_child), schema_objects));
                json_child = json_child->next;
            } while (json_child);
        }
    }
}

void IDFxObject::setProperties(cJSON *cjson_object)
{
    if (cjson_object) {
        cJSON *property(cjson_object->child);
        if (property) {
            while (property) {
                string property_name(property->string);
                switch (property->type) {
                case cJSON_Number: { //get numeric types
                    double property_value((property->valuedouble) ? property->valuedouble : property->valueint);
                    cout << property_name << " : " << property_value << endl;
                    _properties->emplace(property_name, to_string(property_value));
                    break;
                }
                case cJSON_String: { //get alpha types
                    string property_value((property->valuestring) ? property->valuestring : "");
                    cout << property_name << " : " << property_value << endl;
                    _properties->emplace(property_name, property_value);
                    break;
                }
                default: {
                    // nothing to do
                    break;
                }
                }
                property = property->next;
            }
        }
    }
}

std::string IDFxObject::value(string field_name)
{
    auto search(_properties->find(field_name));
    return (search != _properties->end())
           ? search->second
           : "";
}

std::string IDFxObject::value(u_int32_t field_index)
{
//   cout << _schema_object->idd_field(field_index) <<endl;
    return value(_schema_object->idd_field(field_index));
}

// list<std::string> IDFxObject::dataIDF()
// {
//     list<string> return_list;
//     return_list.emplace(return_list.end(), _object_type);
//     if (value("extension_type") != "")
//
//
//     for (int i = 0; i < _property_count; i++) {
//         return_list.emplace(return_list.end(), value(i));
//     }
// //    for(const auto &one_object: getExtensions()) {
// //         for(string one_field: one_object->data()) {
// //             return_list.emplace(return_list.end(), one_field);
// //         }
// //     }
//     return return_list;
// }


/////////////////////////// IDFxObjects  ///////////////////////////////

IDFxObjects::IDFxObjects(): //const string &json_content) :
    _idfx_objects(new vector<IDFxObject* >())
{

}

IDFxObjects::~IDFxObjects()
{

}


void IDFxObjects::insertIDFxObject(IDFxObject *idfx_object)
{
    _idfx_objects->emplace_back(idfx_object);
}





////////////////////// JSONDataInterface ///////////////////////////////



JSONDataInterface::JSONDataInterface(const string &json_schema) :
    _schema_objects(new IDDxObjects(json_schema)),
    _model_objects(new IDFxObjects())
{
    randomize();  //prepare uuid generation function
}

JSONDataInterface::~JSONDataInterface()
{

}


std::map<std::string, IDFxObject* > JSONDataInterface::getModelObjects(std::string object_type)
{
    std::map<std::string, IDFxObject * > return_object_list;



//     cJSON *subitem=item->child;
// 	while (subitem)
// 	{
// 		// handle subitem
// 		if (subitem->child) parse_object(subitem->child);
//
// 		subitem=subitem->next;
// 	}
//







    //     for (int i = 0; i < cJSON_GetArraySize(_model_j); ++i) {
//
//
//         if (subItem) {
//             const string item_type = subItem->string;
//             if (item_type.find(object_type) != std::string::npos ) {
//                 return_object_list.push_back(make_pair<std::string, std::shared_ptr<JSONDataObject> >(item_type, make_shared<JSONDataObject>(cJSON_Print(subItem) add_pointer_to_schema_cJSON ));
//             }
//         } else {
//             cout << "ERROR: subItem not returned" << endl;
//         }
//     }
    return return_object_list;
}

bool JSONDataInterface::exportIDFfile(string filename)
{
    //open file buffer
    ofstream idf_file(filename + ".idf", ofstream::trunc | ofstream::out);
    if (idf_file.is_open()) {
        for (auto & one_object : *_model_objects->objectVector()) {
            //create line of one object
            string this_object_type = one_object->objectType();
            string object_string = this_object_type;
            cout << endl << one_object->propertyCount();
            for (int i = 0; i < one_object->propertyCount(); i++) {
                //  cout << "field string : " << one_object->value(i) << endl << endl;
                object_string.append("," + one_object->value(one_object->value(i)));
            }
            string extension_type = _schema_objects->getIDDxObjectPropertyValue(this_object_type, "extension_type");
//             if (extension_type != "") { //TODO: get extension object once,and iterate it into local variables for inner loop;
// 		for (auto& one_extension_object : one_object->extension
//
//
//             }
            object_string.append(extension_type + ";\n");
            cout << object_string;
            //put line in file buffer
            idf_file << object_string;
        }

//   write file buffer
        idf_file.close();
        return true;
    }
    return false;
}



//std::list<std::vector <std::string> > JSONDataInterface::getOldModelObjects()
//{
//    list<vector <string> > return_list;
//     vector <string> object_data_vector;
//     for (auto & object : getModelObjects()) {
//         auto object_properties = object->getProperties();
//         const string object_type = object_properties.find("object_type")->second;
//         cout << "*****object_type******************           " << object_type << endl;
//         object_data_vector.push_back(object_type + ",");
// //       cJSON *idd_object = cJSON_GetObjectItem(idd_ordered, object_type.c_str());
//         if (idd_object) {
//             unsigned int schema_field_count = 10000;
//             cJSON *schema_object = cJSON_GetObjectItem(_schema_j, object_type.c_str());
//             if (schema_object) {
//                 schema_field_count = cJSON_GetArraySize(schema_object);
//
//                 unsigned int idd_field_count = cJSON_GetArraySize(idd_object);
//                 unsigned int base_object_loop_count = (idd_field_count < schema_field_count) ? idd_field_count : schema_field_count;
//                 unsigned int field_counter;
//                 for (field_counter = 0; field_counter <= base_object_loop_count; ++field_counter) {
//                     cJSON *field_object = cJSON_GetArrayItem(idd_object, field_counter);
//                     string data = "";
// //                 string data_x = "";
//                     if (field_object) {
//                         const string field_name = field_object->valuestring;
//                         auto field_data = object_properties.find(field_name);
//                         if (field_data != object_properties.end()) {
//                             data = field_data->second;
//                             string terminus = (field_counter == base_object_loop_count - 1) ? ";" : ",";
//                             object_data_vector.push_back(data + terminus);
//                         }
//                     }
//                 }
//
//                 string object_type_x = object_type + "_x";
//                 cJSON *schema_x_object = cJSON_GetObjectItem(_schema_j, object_type_x.c_str());
//                 if (schema_x_object) {
//
//                     // make ordered_stringlist of schema_j-extension object field_names based on idd_position
//                     // for(number of extension objects) {
//
//
//                     // for (each ordered field_name stringlist) {
//                     //  append value
//
//
//                     //} //for (each ordered field_name stringlist)
//                     //} //for(number of extension objects)
//
//
// //                     ++field_counter;
//
//
//                 }
//
//
//                 object_data_vector.push_back("");
//             }
//
// //         cout << " ------------------ " << endl;
// //         for (auto & one : object_data_vector) {
// //             cout << one << endl;
// //         }
// //         cout << endl;
// // 	cout << endl << " ------------------ " << endl;
//         }
//         return_list.push_back(object_data_vector);
//
//
//
//     }

//  return return_list;

//put object_type into array at [0]
//for each item in ordered schema object, put matching labeled data into array


//when ordered schema object item is not found in json schema object, switch to extensible

//for each extensible object, process and append data strings to array

//}


void JSONDataInterface::importIDFxFile(string filename)
{
    ifstream idfj(filename.c_str(), std::ifstream::in);
    if (idfj) {
        string json_data = string((std::istreambuf_iterator<char>(idfj)), std::istreambuf_iterator<char>());
        if (json_data != "") {
            if (!importIDFxModel(json_data)) {
                cout << "\nFAILURE: Invalid values detected in imported IDF" << endl;
            }
        } else {
            cout << "\nERROR: JSON data not read from file. " << endl;
        }
    } else {
        cout << "\nERROR: file not open. " << endl;
    }
}


bool JSONDataInterface::importIDFxModel(const string &json_content)
{
    //TODO: maybe move this into the IDFxObjects
    cJSON *data_model = cJSON_Parse(json_content.c_str());
    if (data_model->child) {
        cJSON *mdl(data_model->child);
        if (mdl) {
            cJSON *obj(mdl->child);
            while (obj) {
                auto one_object(new IDFxObject(cJSON_Print(obj), *_schema_objects));
                if (one_object) {
                    _model_objects->insertIDFxObject(one_object);
                } else {
                    cout << "\nFAIL: object creation - " << one_object->objectType() << endl;
                    return false;
                }
                obj = obj->next;
            }
        }
        return validateModel();//always true, at the moment
    } else {
        cout << "\nERROR: failure reading input file: " << endl << cJSON_GetErrorPtr();
        return false;
    }

}



void JSONDataInterface::writeJSONdata(const string &filename)
{
//     ofstream idfj(filename.c_str());
//     if (idfj) {
//         idfj << cJSON_Print(_model_j);
//     }
}

void JSONDataInterface::checkRange(cJSON *attribute, const string &property_name, const string &child_name, bool &valid, double property_value)
{
    if (attribute) {
        string attribute_name = attribute->string;
        if (attribute_name.find("imum")  != std::string::npos) { //only if range checkable fields
            double attribute_value = (attribute->valuestring) ? stod(attribute->valuestring) : attribute->valueint;
            if (attribute_name == "exclude_minimum") {
                if (!(property_value > attribute_value)) {
                    valid = false;
                    cout << "INVALID: " << child_name << " : " << property_name << " : " << property_value <<  " must be > " << attribute_value << endl;
                }
            } else if (attribute_name == "exclude_maximum") {
                if (!(property_value < attribute_value)) {
                    valid = false;
                    cout << "INVALID: " << child_name << " : " << property_name << " : " << property_value <<  " must be < " << attribute_value << endl;
                }
            } else if (attribute_name == "minimum") {
                if (!(property_value >= attribute_value)) {
                    valid = false;
                    cout << "INVALID: " << child_name << " : " << property_name << " : " << property_value <<  " must be >= " << attribute_value << endl;;
                }
            } else if (attribute_name == "maximum") {
                if (!(property_value <= attribute_value)) {
                    valid = false;
                    cout << "INVALID: " << child_name << " : " << property_name << " : " << property_value <<  " must be <= " << attribute_value << endl;
                }
            }
        }
    }
}

void JSONDataInterface::checkNumeric(double property_value, const string &property_name, cJSON *schema_object, bool &valid, const string &child_name)
{
    if (schema_object) {                               //get matching schema property
        cJSON *schema_property = cJSON_GetObjectItem(schema_object, property_name.c_str());
        if (schema_property) {
            for (int a = 0; a < cJSON_GetArraySize(schema_property); a++) {
                cJSON *attribute = cJSON_GetArrayItem(schema_property, a);
                checkRange(attribute, property_name, child_name, valid, property_value);
            }
        }
    }
}

bool JSONDataInterface::validateModel()
{
    // now object validates itself

    bool valid = true;
//     cJSON *child = _model_j->child;
//     if (child) {
//         do {
//             string child_name = child->string;
//             for (int i = 0; i < cJSON_GetArraySize(child); i++) {
//                 cJSON *property = cJSON_GetArrayItem(child, i);
//                 if (property) {
//                     string property_name = property->string;
//                     switch (property->type) {
//                     case cJSON_Number: { //check numeric types
//                         double property_value = (property->valuedouble) ? property->valuedouble : property->valueint;
//                         // now object validates itself   cJSON *schema_object = getSchemaObject(child_name);
//                         checkNumeric(property_value, property_name, schema_object, valid, child_name);
//                         break;
//                     }
//                     case cJSON_String: { //check alpha types
//                         //TBD - check choice and/or reference cases?
//                         break;
//                     }
//                     default: {
//                         // nothing to do
//                         break;
//                     }
//                     }
//                 }
//             }
//             child = child->next;
//         } while (child);
//     } else {
//         cout << "ERROR: model is empty" << endl;
//     }

    return valid;
}




/////////////////////////////// begin  random and uuid gen
//these random functions from here:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3551.pdf
std::default_random_engine &global_urng()
{
    static std::default_random_engine u {};
    return u;
}
void randomize()
{
    static std::random_device rd {};
    global_urng().seed(rd());
}
int randomInRange(int from, int thru)
{
    static std::uniform_int_distribution<> d {};
    using parm_t = decltype(d)::param_type;
    return d(global_urng(), parm_t {from, thru});
}
double randomInRange(double from, double upto)
{
    static std::uniform_real_distribution<> d {};
    using parm_t = decltype(d)::param_type;
    return d(global_urng(), parm_t {from, upto});
}

//return a version 4 (random) uuid
string getUuid()
{
    stringstream uuid_str;
    int four_low = 4096;
    int four_high = 65535;
    int three_low = 256;
    int three_high = 4095;
    uuid_str << std::hex << randomInRange(four_low, four_high);
    uuid_str << std::hex << randomInRange(four_low, four_high);
    uuid_str << "-" << std::hex << randomInRange(four_low, four_high);
    uuid_str << "-" << std::hex << randomInRange(four_low, four_high);
    uuid_str << "-4" << std::hex << randomInRange(three_low, three_high);
    uuid_str << "-8" << std::hex << randomInRange(three_low, three_high);
    uuid_str << std::hex << randomInRange(four_low, four_high);
    uuid_str << std::hex << randomInRange(four_low, four_high);
    return uuid_str.str();
}
/////////////////////// end random and uuid gen
///
///
// from here down...
//utility functions only used in old IDF text import

std::vector<std::string> insertStringSplit(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::string item;
    std::stringstream ss(s);
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    //expedient measure to ensure vector has three values
    elems.push_back("");
    elems.push_back("");
    elems.push_back("");
    return elems;
}

std::vector<std::string> splitString(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::string item;
    std::stringstream ss(s);
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::string &trimString(std::string &str)
{
    str.erase(str.begin(), find_if(str.begin(), str.end(),
                                   [](char & ch)->bool { return !isspace(ch); }));
    str.erase(find_if(str.rbegin(), str.rend(),
                      [](char & ch)->bool { return !isspace(ch); }).base(), str.end());
    return str;
}

vector<string> getFileObjectStrings(string filename)
{
    string oneline = "";
    string nextline = "";
    ifstream idf(filename.c_str(), std::ifstream::in);
    while (getline(idf, nextline)) {
        //reduce file to list of actual object instances and their fields
        if (nextline != "") { //discard blank lines that make bits.at(0) fail
            vector<string> bits = splitString(nextline, '!');
            string split = bits.at(0);//drop all after remark
            string checkblank = trimString(split);
            if (checkblank != "")
                oneline.append(checkblank);
        }
    }
    return splitString(oneline, ';');
}

void insertTypedData(vector<string> obj_props, cJSON *current_object, unsigned int field_counter, const char *object_type, const char *field_name, cJSON *attribute_data)
{
    if (attribute_data) {
        cJSON *data_type_obj = cJSON_GetObjectItem(attribute_data, "data_type");
        string data_value = "";
        try {
            data_value = obj_props.at(field_counter).c_str();
        } catch (std::out_of_range) {
            ;// really do nothing except not crash, when the final field is blank;
        }
        if (data_type_obj) {
            string data_type = data_type_obj->valuestring;
            if (data_value != "") { //exclude empty fields
                if ((data_type == "real") || (data_type == "integer")) {
                    if (data_type == "integer") {
                        try {
                            cJSON_AddNumberToObject(current_object, field_name, stoi(data_value));
                        } catch (std::invalid_argument) {
                            cJSON_AddStringToObject(current_object, field_name, data_value.c_str());
                            //since practically any numeric value can get a non-numeric value(auto... or parametric),
                            //we can't call this a failure, we simply have to write it as a string
                        }
                    }
                    if (data_type == "real") {
                        try {
                            cJSON_AddNumberToObject(current_object, field_name, stod(data_value));
                        } catch (std::invalid_argument) {
                            cJSON_AddStringToObject(current_object, field_name, data_value.c_str());
                            //since practically any numeric value can get a non-numeric value(auto... or parametric),
                            //we can't call this a failure, we simply have to write it as a string
                        }
                    }
                } else { //all others are string
                    cJSON_AddStringToObject(current_object, field_name, data_value.c_str());
                }
            } //empty, continue
        } else { //all others are string, even those without data_type
            cout << "FAIL : data_type not found. " <<  object_type << " : " << field_name << endl;
        }
    } else {
        cout << "FAIL : attribute_data = " <<  object_type << endl;
    }
}

void insertField(cJSON *full_schema_object, const char *object_type, unsigned int field_counter, vector<string> obj_props, cJSON *field_object, cJSON *current_object)
{
    if (field_object) {
        const char *field_name = field_object->valuestring;
        cJSON *attribute_data = cJSON_GetObjectItem(full_schema_object, field_name);
        insertTypedData(obj_props, current_object, field_counter, object_type, field_name, attribute_data);
    } else {
        cout << "FAIL : field_object = " <<  object_type << endl;
    }
}


void JSONDataInterface::importIDFFile(string filename)
{
//     vector<string> object_instance_lines = getFileObjectStrings(filename);
//     for (string objstr : object_instance_lines) {
//         auto obj_props = splitString(objstr, ',');
//         string object_string = obj_props.at(0);
//         //rudimentary capture of Version
//         if ((object_string == "Version") || (object_string == "VERSION")) {
//             string version = obj_props.at(1);
//             if (version != ACCEPTED_VERSION) {
//                 cout << "idfxt TRANSLATOR ONLY WORKS ON VERSION 8.2 IDF INPUT FILES" << endl << "this file version is:  " << version << endl;
//                 break;
//             }
//         }
//         const char *object_type = object_string.c_str();
// //       cJSON *schema_object = cJSON_GetObjectItem(idd_ordered, object_type);
//         if (schema_object) {
//             unsigned int schema_field_count = cJSON_GetArraySize(schema_object);
//             unsigned int model_field_count = obj_props.size() - 1;
//             unsigned int base_field_count = (model_field_count < schema_field_count) ? model_field_count : schema_field_count;
//             cJSON *current_object = cJSON_CreateObject();
//             if (current_object) {
//                 cJSON_AddItemToObject(getModelRootObject(), getUuid().c_str(), current_object);
//                 cJSON_AddStringToObject(current_object, "object_type", object_type);
//                 //set this loop count for base object
//                 cJSON *full_schema_object = getSchemaObject(object_type);
//                 if (full_schema_object) {
//                     unsigned int field_counter;
//                     for (field_counter = 1; field_counter <= base_field_count; ++field_counter) {
//                         cJSON *field_object = cJSON_GetArrayItem(schema_object, field_counter - 1);
//                         insertField(full_schema_object, object_type, field_counter, obj_props, field_object, current_object);
//                     }
//                     // if obj_props remain, use 'em up in extension objects
//                     string extension_test_string = object_type;
//                     extension_test_string.append("_x");  //TODO: extract value from object's "extension_type", if name convention changes
// //                    cJSON *extension_type = cJSON_GetObjectItem(idd_ordered, extension_test_string.c_str());
//                     if (extension_type) {
//                         cJSON *extension_array = cJSON_CreateArray();
//                         cJSON_AddItemToObject(current_object, "extensions", extension_array);
//                         unsigned int field_total = obj_props.size();
//                         while (field_counter < field_total) {
//                             cJSON *extension_object = cJSON_CreateObject();
//                             if (extension_object) {
//                                 int size = cJSON_GetArraySize(extension_type);
//                                 for (int f = 0; f < size; ++f) {
//                                     cJSON *extension_field = cJSON_GetArrayItem(extension_type, f);
//                                     if (extension_field) {
//                                         const char *extension_field_name = extension_field->valuestring;
//                                         cJSON *schema_extension_object = getSchemaObject(extension_test_string.c_str());
//                                         if (schema_extension_object) {
//                                             cJSON *field_data = cJSON_GetObjectItem(schema_extension_object, extension_field_name);
//                                             if (field_data) {
//                                                 insertTypedData(obj_props, extension_object, field_counter, extension_test_string.c_str(), extension_field_name, field_data);
//                                             }
//                                         }
//                                         field_counter++;
//                                     }
//                                 }
//                                 cJSON_AddItemToArray(extension_array, extension_object);
//                             }
//                         }
//                     }
//                 }
//             } else {
//                 cout << "ERROR: type \"" << object_type << "\" is not found in E+ " << ACCEPTED_VERSION << " schema." << endl;
//             }
//         }
//     }
}
