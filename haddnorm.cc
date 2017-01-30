#include <string.h>
#include "TChain.h"
#include "TFile.h"
#include "TH1.h"
#include "TTree.h"
#include "TKey.h"
#include "Riostream.h"
TList *FileList;
TFile *Target;
double calculateSumOfNormHist( string normHistName, TList *sourcelist );
void MergeRootfile( TDirectory *target, TList *sourcelist, double weight );

using std::string;
using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  if (argc<4) {
		cout << "\n======================== USAGE INFO ===============================================" << endl;
		cout << "       ./haddws normHist output.root file1.root file2.root ..." << endl;
		cout << "          1 in any error, 0 upon successful completion." << endl;
		cout << "======================== END USAGE INFO =============================================\n" << endl;
    return 1;
  }
  string normHistName(argv[1]);
  string outputFileName(argv[2]);
	vector<string> inputFileNames;
  Target = TFile::Open( outputFileName.c_str(), "NEW" );
  if (!Target) {
    cout << "Output file: " << outputFileName << " already exists! Aborting...\n";
  }
  FileList = new TList();
  for (int i=3; i< argc; ++i) {
    inputFileNames.push_back((argv[i]));
    FileList->Add( TFile::Open(argv[i]) );
    cout << argv[i] << endl;
  }

  double normHistSum = 0;
  normHistSum = calculateSumOfNormHist(normHistName, FileList);
  cout << "normHistSum: " << normHistSum << endl;
  // if (normHistSum>0) MergeRootfile(Target, FileList, 1/normHistSum);
  // else cout << "Normalization must be non-zero" << endl;
  return 1;

}

double calculateSumOfNormHist( string normHistName, TList *sourcelist ) {
  double sum = 0;

  TFile *first_source = (TFile*)sourcelist->First();
  cout << "first_source: " << first_source << endl;
  first_source->cd();
  TDirectory *current_sourcedir = gDirectory;
  //gain time, do not add the objects in the list in memory
  Bool_t status = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  TIter nextkey( current_sourcedir->GetListOfKeys() );
  TKey *key, *oldkey=0;
  while ( (key = (TKey*)nextkey())) {
    //keep only the highest cycle number for each key
    if (oldkey && !strcmp(oldkey->GetName(),key->GetName())) continue;
    TObject *obj = key->ReadObj();
    if ( obj->IsA()->InheritsFrom( TH1::Class() ) && obj->GetName()==normHistName ) {
      sum += ((TH1*)obj)->Integral();
      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      while ( nextsource ) {
        nextsource->cd();
        TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(normHistName.c_str());
        if (key2) {
          TH1 *h2 = (TH1*)key2->ReadObj();
          sum += h2->Integral();
        }
        nextsource = (TFile*)sourcelist->After( nextsource );
      }
    }
  }
  return sum;
}

void MergeRootfile( TDirectory *target, TList *sourcelist, double weight ) {
   //  cout << "Target path: " << target->GetPath() << endl;
   TString path( (char*)strstr( target->GetPath(), ":" ) );
   path.Remove( 0, 2 );
   TFile *first_source = (TFile*)sourcelist->First();
   first_source->cd( path );
   TDirectory *current_sourcedir = gDirectory;
   //gain time, do not add the objects in the list in memory
   Bool_t status = TH1::AddDirectoryStatus();
   TH1::AddDirectory(kFALSE);
   // loop over all keys in this directory
   TChain *globChain = 0;
   TIter nextkey( current_sourcedir->GetListOfKeys() );
   TKey *key, *oldkey=0;
   while ( (key = (TKey*)nextkey())) {
      //keep only the highest cycle number for each key
      if (oldkey && !strcmp(oldkey->GetName(),key->GetName())) continue;
      // read object from first source file
      first_source->cd( path );
      TObject *obj = key->ReadObj();
      if ( obj->IsA()->InheritsFrom( TH1::Class() ) ) {
         // descendant of TH1 -> merge it
         //      cout << "Merging histogram " << obj->GetName() << endl;
         TH1 *h1 = (TH1*)obj;
         h1->Scale(weight);
         // loop over all source files and add the content of the
         // correspondant histogram to the one pointed to by "h1"
         TFile *nextsource = (TFile*)sourcelist->After( first_source );
         while ( nextsource ) {
            // make sure we are at the correct directory level by cd'ing to path
            nextsource->cd( path );
            TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(h1->GetName());
            if (key2) {
               TH1 *h2 = (TH1*)key2->ReadObj();
               h2->Scale(weight);
               h1->Add( h2 );
               delete h2;
            }
            nextsource = (TFile*)sourcelist->After( nextsource );
         }
      }
      else if ( obj->IsA()->InheritsFrom( TTree::Class() ) ) {
         // loop over all source files create a chain of Trees "globChain"
         const char* obj_name= obj->GetName();
         globChain = new TChain(obj_name);
         globChain->Add(first_source->GetName());
         TFile *nextsource = (TFile*)sourcelist->After( first_source );
         //      const char* file_name = nextsource->GetName();
         // cout << "file name  " << file_name << endl;
         while ( nextsource ) {
            globChain->Add(nextsource->GetName());
            nextsource = (TFile*)sourcelist->After( nextsource );
         }
      } else if ( obj->IsA()->InheritsFrom( TDirectory::Class() ) ) {
         // it's a subdirectory
         cout << "Found subdirectory " << obj->GetName() << endl;
         // create a new subdir of same name and title in the target file
         target->cd();
         TDirectory *newdir = target->mkdir( obj->GetName(), obj->GetTitle() );
         // newdir is now the starting point of another round of merging
         // newdir still knows its depth within the target file via
         // GetPath(), so we can still figure out where we are in the recursion
         MergeRootfile( newdir, sourcelist, weight );
      } else {
         // object is of no type that we know or can handle
         cout << "Unknown object type, name: "
         << obj->GetName() << " title: " << obj->GetTitle() << endl;
      }
      // now write the merged histogram (which is "in" obj) to the target file
      // note that this will just store obj in the current directory level,
      // which is not persistent until the complete directory itself is stored
      // by "target->Write()" below
      if ( obj ) {
         target->cd();
         //!!if the object is a tree, it is stored in globChain...
         if(obj->IsA()->InheritsFrom( TTree::Class() ))
            globChain->Merge(target->GetFile(),0,"keep");
         else
            obj->Write( key->GetName() );
      }
   } // while ( ( TKey *key = (TKey*)nextkey() ) )
   // save modifications to target file
   target->SaveSelf(kTRUE);
   TH1::AddDirectory(status);
}
