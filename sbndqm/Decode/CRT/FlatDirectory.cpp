//File: FlatDirectory.h
//Brief: FlatDirectory wraps over any class that implements a contract like 
//       art::TFileDirectory to expose the same contract without creating 
//       any sub-TDirectoires.  Instead, FlatDirectory::make<>() appends its 
//       name to objects' names to give some strong assurances of name 
//       uniqueness.  FlatDirectory might be useful in adapting code that 
//       normally relies on TFileDirectory-like objects to work with 
//       post-processing software like Monet that can't deal with nested 
//       TDirectories. 
//Author: Andrew Olivier aolivier@ur.rochester.edu

//c++ includes
#include <memory> //For std::shared_ptr
#include <string> //For std::string

#ifndef CRT_FLATDIRECTORY_CPP
#define CRT_FLATDIRECTORY_CPP
namespace CRT //Using namespace CRT just to avoid name pollution for now.  
              //Feel free to move to a more appropriate/descriptive namespace 
              //if in wider use.  FlatDirectory really has nothing to do 
              //with the ProtoDUNE-SP CRT.
{
  //A DIRPTR has pointer syntax that accesses an object that looks like:
  //
  //template <class HIST, class ARGS...> HIST* make(ARGS... args)
  //
  //Looks like art::ServiceHandle<art::TFileService>.  
  template <class DIRPTR>
  class FlatDirectory //A FlatDirectory IS NOT A DIRECTORY because that would 
                      //require that DIRECTORY is copiable and introduce 
                      //coupling to DIRECTORY's source code in other ways.  
  {
    public:
      //Create a top-level FlatDirectory to wrap over some DIRECTORY 
      //object.  A FlatDirectory created this way will not append 
      //anything to the names of its children, but its sub-directories 
      //will.  
      FlatDirectory(DIRPTR dir);
      virtual ~FlatDirectory() = default;
      
      //Implementation of TFileDirectory-like contract.  
      template <class TOBJECT, class ...ARGS>
      TOBJECT* make(ARGS... args)
      {
        //TODO: Figure out which parameter(s) will set obj's name 
        //      without actually creating an object with that name.  
        //      This would avoid some temporary name conflicts that 
        //      would occur with a "real" TDirectory.  
        auto obj = fBaseDir->template make<TOBJECT>(args...);
        const std::string oldName = obj->GetName();  
        obj->SetName((fName+oldName).c_str());
        return obj;
      }

      template <class TOBJECT, class ...ARGS>
      TOBJECT* makeAndRegister(const std::string& name, const std::string& title, ARGS... args)
      {
        return fBaseDir->template makeAndRegister<TOBJECT>(fName+name, title, args...);
      }

      FlatDirectory mkdir(const std::string& name);
       
    private:
      DIRPTR fBaseDir; //Base directory in which this 
                       //FlatDirectory and all of its 
                       //children will put objects they 
                       //make<>().  

      const std::string fName; //The name of this FlatDirectory.  Will be 
                               //appended to the names of all child objects
                               //to create unique names.  

      //Create a subdirectory of a given FlatDirectory.  This behavior is 
      //exposed to the user through mkdir.
      FlatDirectory(const std::string& name, FlatDirectory& parent);

      static constexpr auto Separator = "_"; //The separator between nested FlatDirectories' names.  
                                             //I'll probably never change it, but I've written a 
                                             //parameter here so that any future changes can be 
                                             //maintained in one place.  
                                             //
                                             //Worrying so much about the storage "policy" for 
                                             //what is probably just a C-string is serious overkill.  
  };

  //Define member functions out of class body for cleanliness
  template <class DIRPTR>
  FlatDirectory<DIRPTR>::FlatDirectory(DIRPTR dir): fBaseDir(dir), fName("")
  {
  }
                                                                                                                              
  template <class DIRPTR>
  FlatDirectory<DIRPTR> FlatDirectory<DIRPTR>::mkdir(const std::string& name)
  {
    FlatDirectory child(name, *this);
    return child;
  }
                                                                                                                              
  template <class DIRPTR>
  FlatDirectory<DIRPTR>::FlatDirectory(const std::string& name, FlatDirectory& parent): fBaseDir(parent.fBaseDir), 
                                                                                        fName(parent.fName+name+Separator)
  {
  }

  //For pre-c++17, syntatic sugar to create a FlatDirectory from anything that implements the 
  //art::TFileDirectory contract.
  //TODO: To be pedantic, maybe a deprecated attribute if compiler supports c++17?
  template <class DIRPTR>
  #if __cplusplus > 201402L
  [[deprecated("In c++17, the compiler should infer template parameters for you in the FlatDirectory<> constructor.")]]
  #endif
  FlatDirectory<DIRPTR> make_FlatDirectory(DIRPTR& ptr)
  {
    return FlatDirectory<DIRPTR>(ptr);
  }
}

#endif //CRT_FLATDIRECTORY_CPP  
