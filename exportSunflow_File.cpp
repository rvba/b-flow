/* * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Milovann Yanatchkov - mil01 at free dot fr  
 *
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/* --== SUNFLOW==-- */

#include "BLI_arithb.h"
#include "BKE_main.h"

/*#ifdef __cplusplus
extern "C" {
#endif
#include "BSE_headerbuttons.h"
void update_for_newframe();
#ifdef __cplusplus
}
#endif*/

#include "exportSunflow_File.h"
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

map<int, string>  modifiers;	
map<int, string>  textures;	
map<int, string>  materials;
int indice = 0;

/* *********************************************************************************************************************	*/
void SunflowFileRender_t::writeTextures() {}
void SunflowFileRender_t::writeObject(Object*, ObjectRen*, const std::vector<VlakRen*, std::allocator<VlakRen*> >&, const float (*)[4]) {}
void SunflowFileRender_t::writeAreaLamp(LampRen*, int, float (*)[4]) {}
void SunflowFileRender_t::writeHemilight() {}
void SunflowFileRender_t::writePathlight() {}
bool SunflowFileRender_t::writeRender() { return true; }
/* *********************************************************************************************************************************************	*/
/* MISC			************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/

static string unixYafrayPath()
{
	static char *alternative[]=
	{
		"/usr/local/bin/",
		"/usr/bin/",
		"/bin/",
		NULL
	};

	for(int i=0;alternative[i]!=NULL;++i)
	{
		string fp=string(alternative[i])+"sunflow";
		struct stat st;
		if(stat(fp.c_str(),&st)<0) continue;
		if(st.st_mode&S_IXOTH) return alternative[i];
	}
	return "";
}

/* CLEAN NAME ******************************************************************************************************************	*/
string cleanName(string str)
{
    for(unsigned int i=0;i<str.length();i++)
        if (str[i]== '.' || str[i]== ' ' || str[i]== '-' ||str[i]== '+' ||str[i]== '/' ||str[i]== '*' ||str[i]== '&')
            str[i]='_';
    return str;
}
/* FIND TEXT *************************************************************************** */
bool SunflowFileRender_t::findText()
{
	Text *txt_iter;
	txt_iter = (Text *)G.main->text.first;

	int total=0;

	while( ( txt_iter ) )
	{
		if( strcmp( "sunflow", txt_iter->id.name+2) == 0 )
		{
            		Text_sunflow=txt_iter;
			total++;
			return true;
        	}
        	
		txt_iter = (Text *)txt_iter->id.next;
	}
	if ( total > 0 ) { return true ; }
	else  { return false ; }
}
/* GET USER TEXT *************************************************************************** */
void SunflowFileRender_t::getUserText(Text* Text_sunflow)
{
	TextLine *line = ( TextLine * )Text_sunflow->lines.first;

	if ((Text_sunflow->nlines!=1) || (line->len!=0))
	{
		while (line)
			{
				sunflowFile << line->line <<endl;
            			line=line->next;
			}
	}
}
/* DISPLAY IMAGE *************************************************************************** */
void SunflowFileRender_t::displayImage()
{
}
/* *********************************************************************************************************************************************	*/
/* RENDER		************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/
static string command_path = "";

/* INIT EXPORT **************************************************************************************************************	*/
bool SunflowFileRender_t::initExport() 
{ 
	printf("==INIT EXPORT==\n");

	// PATHS
	xmlpath = "";
	bool dir_failed = false;
	// try the user setting setting first, export dir must be set and exist
	if (strlen(U.yfexportdir)==0) 
	{
		cout << "No export directory set in user defaults!" << endl;
		char* temp = getenv("TEMP");
		// if no envar, use /tmp
		xmlpath = temp ? temp : "/tmp";
		cout << "Will try TEMP instead: " << xmlpath << endl;
		// no fail here, but might fail when opening file...
	}
	else 
	{
		xmlpath = U.yfexportdir;
		//adjustPath(xmlpath);	// possibly relative
		cout << "YFexport path is: " << xmlpath << endl;
		// check if it exists
		if (!BLI_exists(const_cast<char*>(xmlpath.c_str()))) {
			cout << "YafRay temporary xml export directory:\n" << U.yfexportdir << "\ndoes not exist!\n";
			dir_failed = true;
		}
	}
	if (command_path=="")
	{
		command_path = unixYafrayPath();
		if (command_path.size()) cout << "Yafray found at : " << command_path << endl;
	}
	// for all
	if (dir_failed) return false;

	string DLM = "/";
	// remove trailing slash if needed
	if (xmlpath.find_last_of(DLM)!=(xmlpath.length()-1)) xmlpath += DLM;

	std::string image_type;
	if ( G.scene->r.imtype == 0 ) image_type = ".tga";
	//else if ( G.scene->r.imtype == 4 ) image_type = ".jpg";
	else if ( G.scene->r.imtype == 17 ) image_type = ".png";
	else if ( G.scene->r.imtype == 23 ) image_type = ".exr";
	else if ( G.scene->r.imtype == 21 ) image_type = ".hdr";
	else image_type = ".png";

	export_image = xmlpath ;
	export_image += "output";
	export_image += image_type;
	export_scene = xmlpath + "sunflow.sc";
	export_geo = xmlpath + "sunflow.geo.sc";

	
	// OPEN FILES
	sunflowFile.open(export_scene.c_str());
	if ( re->r.SF_export_mesh ) cout << "not exporting mesh" << endl ;
	else if ( re->r.SF_MASK ) cout << " " << endl;
	else sunflowMesh.open(export_geo.c_str());

	// ADD TEXT
	if (  findText()  )
       	{
	      	printf("text found\n");
		sunflowFile << "/* TEXT ****************************** */" <<endl;
		getUserText(Text_sunflow);
       	}
	else
	{
		printf("no text\n");
	}
	sunflowFile << "/* SUNFLOW ****************************** */" <<endl;

	// IMAGE
	sunflowFile << "image {" << endl;
	sunflowFile << "\tresolution" 	<< " " << re->rectx << " " << re->recty<< endl;
	sunflowFile << "\taa"		<< " " << re->r.SF_aa_1 << " " << re->r.SF_aa_2 << endl;
	sunflowFile << "\tsamples" 	<< " " << re->r.SF_aa_samples << endl;
	sunflowFile << "\tcontrast"	<< " " << re->r.SF_contrast << endl;
	sunflowFile << "\tfilter"	<< " " << "box" << endl;
	if ( re->r.SF_jitter )
	sunflowFile << "\tjitter true" << endl;
	else
	sunflowFile << "\tjitter false" << endl;
	sunflowFile << "}" << endl;

	// TRACE DEPTH
	sunflowFile << "trace-depths {" << endl;
	sunflowFile << "\tdiff" << " " << re->r.SF_DEPTH_diff << endl;
	sunflowFile << "\trefl" << " " << re->r.SF_DEPTH_refl << endl;
	sunflowFile << "\trefr" << " " << re->r.SF_DEPTH_refr << endl;
	sunflowFile << "}" << endl;

	// BASIC SHADERS
	if ( G.scene->r.SF_MASK )
		sunflowFile << "shader {\n\tname\tdef\n\ttype constant\n\tcolor .5 .5 .5\n}\n"<<endl;
	else
		sunflowFile << "shader {\n\tname\tdef\n\ttype diffuse\n\tdiff 1 1 1\n}\n"<<endl;

	sunflowFile << "shader {\n\tname debug_globals\n\ttype view-global\n}\n"<<endl;
	sunflowFile << "shader {\n\tname debug_gi\n\ttype view-irradiance\n}\n"<<endl;

	// BUCKET
	if (re->r.SF_bucket_type == 0 ) 
		sunflowFile << "bucket" << " " << re->r.SF_bucket_size << " " << "hilbert" << endl;
	if (re->r.SF_bucket_type == 1 ) 
		sunflowFile << "bucket" << " " << re->r.SF_bucket_size << " " << "spiral" << endl;
	if (re->r.SF_bucket_type == 2 ) 
		sunflowFile << "bucket" << " " << re->r.SF_bucket_size << " " << "column" << endl;
	if (re->r.SF_bucket_type == 3 ) 
		sunflowFile << "bucket" << " " << re->r.SF_bucket_size << " " << "row" << endl;
	if (re->r.SF_bucket_type == 4 ) 
		sunflowFile << "bucket" << " " << re->r.SF_bucket_size << " " << "diagonal" << endl;
	if (re->r.SF_bucket_type == 5 ) 
		sunflowFile << "bucket" << " " << re->r.SF_bucket_size << " " << "random" << endl;
	sunflowFile << "\n" ;

	// IGI
	if (re->r.SF_GI_type == 0 )
	{
		sunflowFile << "gi {" << endl;
		sunflowFile << "\ttype" 	<< " " << "igi" <<endl;
		sunflowFile << "\tsamples" 	<< " " << re->r.SF_IGI_samples << endl;
		sunflowFile << "\tsets" 	<< " " << re->r.SF_IGI_sets << endl;
		sunflowFile << "\tb" 		<< " " << re->r.SF_IGI_bias << endl;
		sunflowFile << "\tbias-samples"	<< " " << re->r.SF_IGI_bias_samples << endl;
		sunflowFile << "}" << endl;
	}
	// IRRADIANCE CACHING 
	if (re->r.SF_GI_type == 1 )
	{
		sunflowFile << "gi {" << endl;
		sunflowFile << "\ttype" 	<< " " << "irr-cache" <<endl;
		sunflowFile << "\tsamples" 	<< " " << re->r.SF_IRR_samples << endl;
		sunflowFile << "\ttolerance" 	<< " " << re->r.SF_IRR_tolerance << endl;
		sunflowFile << "\tspacing" 	<< " " << re->r.SF_IRR_spacing_1 << " " << re->r.SF_IRR_spacing_2 << endl;
		sunflowFile << "\tglobal"	<< " " << re->r.SF_IRR_global * 1000 << " " ;
		sunflowFile << "grid"	<< " " << re->r.SF_IRR_grid_1 << " " << re->r.SF_IRR_grid_2 << endl;
		sunflowFile << "}" << endl;
	}
	// PATH TRACING 
	if (re->r.SF_GI_type == 2 )
	{
		sunflowFile << "gi {" << endl;
		sunflowFile << "\ttype" 	<< " " << "path" <<endl;
		sunflowFile << "\tsamples" 	<< " " << re->r.SF_PATH_samples << endl;
		sunflowFile << "}" << endl;
	}
	// AMBIENT OCCLUSION 
	if (re->r.SF_GI_type == 3 )
	{
		sunflowFile << "gi {" << endl;
		sunflowFile << "\ttype" 	<< " " << "ambocc" <<endl;
		sunflowFile << "\tbright" 	<< " " << re->r.SF_AMB_B_r << " "  << re->r.SF_AMB_B_g << " "  << re->r.SF_AMB_B_b << endl; 
		sunflowFile << "\tdark" 	<< " " << re->r.SF_AMB_D_r << " "  << re->r.SF_AMB_D_g << " "  << re->r.SF_AMB_D_b << endl; 
		sunflowFile << "\tsamples" 	<< " " << re->r.SF_AMB_samples << endl;
		sunflowFile << "\tmaxdist" 	<< " " << re->r.SF_AMB_maxdist << endl;
		sunflowFile << "}" << endl;
	}
	// FAKE AMBIENT 
	if (re->r.SF_GI_type == 4 )
	{
		sunflowFile << "gi {" << endl;
		sunflowFile << "\ttype" 	<< " " << "fake" <<endl;
		sunflowFile << "\tup" 		<< " " << re->r.SF_FAKE_x << " "  << re->r.SF_FAKE_y << " "  << re->r.SF_FAKE_z << endl; 
		sunflowFile << "\tsky" 		<< " " << re->r.SF_FAKE_S_r << " "  << re->r.SF_FAKE_S_g << " "  << re->r.SF_FAKE_S_b << endl; 
		sunflowFile << "\tground" 	<< " " << re->r.SF_FAKE_G_r << " "  << re->r.SF_FAKE_G_g << " "  << re->r.SF_FAKE_G_b << endl; 
		sunflowFile << "}" << endl;
	}
	//SHOW
	
	if (re->r.SF_GI_view_photons )
		sunflowFile << "override debug_globals false" << endl;
	if ( re->r.SF_GI_view_irradiance )
		sunflowFile << "override debug_gi false" << endl;

	return true;
}
/* FINISH EXPORT **************************************************************************************************************	*/
bool SunflowFileRender_t::finishExport()
{
	cout << "==FINISH EXPORT==\n" << endl;

	// CLOSE FILES
	sunflowFile << "include" << " " << "sunflow.geo.sc" << endl;
	sunflowFile.close();

	if ( re->r.SF_export_mesh ) cout << "" << endl ;
	else sunflowMesh.close();

	if ( executeYafray("none") )	displayImage();
	cout << "= --==WOLFNUS==-- ============================================================\n" << endl;
	return true;
}
/* SUNFLOW FILE RENDER ******************************************************************************************************	*/
bool SunflowFileRender_t::executeYafray(const string &xmlpath)
{
	printf("== -::SunFlow::- ==\n");
	std::string command;
	int ret;	

	command += "java -server -Xmx2048M -jar ";
	command += command_path;
	command += "sunflow";
	command += " -o ";
	if ( xmlpath.compare("none") == 0 )
       	command += export_image;
	else
	command += xmlpath;
	command += " ";
	command += export_scene;  
	
	if ( re->r.SF_QUICK_uvs)
		command += " -quick_uvs "; 
	if ( re->r.SF_QUICK_normals)
		command += " -quick_normals "; 
	if ( re->r.SF_QUICK_id)
		command += " -quick_id " ;
	if ( re->r.SF_QUICK_prims)
		command += " -quick_prims "; 
	if ( re->r.SF_QUICK_gray)
		command += " -quick_gray " ;
	if ( re->r.SF_QUICK_wire)
		command += " -quick_wire -aa 0 2 -filter box "; 
	if ( re->r.SF_QUICK_ambocc)
	{
		command += " -quick_ambocc " ;
		/*
		if ( re->r.SF_AMB_maxdist )
		command += re->r.SF_AMB_maxdist;
		else
		*/
		command += "10";
	}
	if ( re->r.SF_QUICK_nogui)
		command += " -nogui" ;

	if (re->r.SF_ipr)
	      cout << " no ipr " << endl; 	
	else 
		command += " -ipr ";

	cout << "command: " << command << endl;
	ret = system(command.c_str());
	printf("\n");
	printf("done!\n");

	return ret==0;
}
/* *********************************************************************************************************************************************	*/
/* MESH			************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/

/* WRITE ALL OBJECTS	*********************************************************************************************************	*/
void SunflowFileRender_t::writeAllObjects()
{
	printf("==WRITE ALL OBJECTS==\n");
	transform = 1;	
	if ( re->r.SF_export_mesh )
	{
		return;
	}

	sunflowMesh<<"/* OBJECTS ****************************** */\n"<<endl;

	for (map<Object*, yafrayObjectRen >::const_iterator obi=all_objects.begin();
			obi!=all_objects.end(); ++obi)

	{
		// skip main duplivert object if in dupliMtx_list, written later
		Object* obj = obi->first;
		
		if (dupliMtx_list.find(string(obj->id.name))!=dupliMtx_list.end())
		{
        		writeObjectSunflow(obj, obi->second.obr,obi->second.faces, obj->obmat);
		}
        	writeObjectSunflow(obj, obi->second.obr,obi->second.faces, obj->obmat);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Now all duplivert objects (if any) as instances of main object
	// The original object has been included in the VlakRen renderlist above (see convertBlenderScene.c)
	// but is written here which all other duplis are instances of.
	float obmat[4][4], cmat[4][4], imat[4][4], nmat[4][4];
	for (map<string, vector<float> >::const_iterator dupMtx=dupliMtx_list.begin();
		dupMtx!=dupliMtx_list.end();++dupMtx) 
	{

		transform = 0;
		// original inverse matrix, not actual matrix of object, but first duplivert.
		
		for (int i=0;i<4;i++)
			for (int j=0;j<4;j++)
				obmat[i][j] = dupMtx->second[(i<<2)+j];

		MTC_Mat4Invert(imat, obmat);

		// first object written as normal (but with transform of first duplivert)
		Object* obj = dup_srcob[dupMtx->first];
		//writeObject(obj, all_objects[obj].obr, all_objects[obj].faces, obmat);
		writeObjectSunflow(obj, all_objects[obj].obr, all_objects[obj].faces, obmat);

		// all others instances of first
		for (unsigned int curmtx=16;curmtx<dupMtx->second.size();curmtx+=16) 
		{	// number of 4x4 matrices

			// new mtx
			for (int i=0;i<4;i++)
				for (int j=0;j<4;j++)
					nmat[i][j] = dupMtx->second[curmtx+(i<<2)+j];

			MTC_Mat4MulMat4(cmat, imat, nmat);	// transform with respect to original = inverse_original * new

			
			sunflowMesh << "instance {" << endl;
			sunflowMesh << "\tname" << " " << obj->id.name << (curmtx>>4) << endl;
			sunflowMesh << "\tgeometry" << " " << obj->id.name << endl;;
			sunflowMesh << "\ttransform row" 	<< " " << cmat[0][0] << " " << cmat[1][0] << " " << cmat[2][0] << " " << cmat[3][0]
	       							<< " " << cmat[0][1] << " " << cmat[1][1] << " " << cmat[2][1] << " " << cmat[3][1]
							       	<< " " << cmat[0][2] << " " << cmat[1][2] << " " << cmat[2][2] << " " << cmat[3][2]
					        		<< " " << cmat[0][3] << " " << cmat[1][3] << " " << cmat[2][3] << " " << cmat[3][3] << endl ;
			sunflowMesh << "\tshader def" << endl;
			sunflowMesh << "}" << endl;

			//VlakRen* face0 = all_objects[obj].faces[0];
			//Material* face0mat = face0->mat;
			//string matname(face0mat->id.name);
			//string userLib=getUserMat(matname,PovMat,false);
		}

	}
	printf("done!\n");
}
/* WRITE OBJECT SUNFLOW *************************************************************************** */
void SunflowFileRender_t::writeObjectSunflow(	Object* obj,				// obj			Object
						ObjectRen *obr,				// obr			Object Ren
					       	const vector<VlakRen*> &VLR_list,	// VLR_list		Vector
					       	const float obmat[4][4]			// obmat		Matrix
						)
{
	printf(".");		// debug 
	//int transform = 1;	// transform
	int meshlight =0 ;	// meshlight
	int object = 1;		// object

	// ==MESHLIGHT====================================================================================================

	if ( obj->SF_meshlight ){ meshlight = 1 ; object = 0; }
	
	if ( meshlight )
	{
		sunflowMesh << "light {"<< endl;
		sunflowMesh <<"\ttype meshlight" << endl;
		sunflowMesh << "\tname " << cleanName(obj->id.name) << endl;
		sunflowMesh << "\temit" << " {\"sRGB nonlinear\"" << " " << obj->SF_meshlight_r << " " << obj->SF_meshlight_g << " " << obj->SF_meshlight_b << " }" << endl;
		sunflowMesh <<"\tradiance" << " " << obj->SF_meshlight_radiance << endl;
		sunflowMesh <<"\tsamples" << " " << obj->SF_meshlight_samples << endl;
	}
	// === OBJECT ===================================================================================================
	else
		sunflowMesh<<"object {"<<endl;
	 
	//
	// ***SHADER*******************************************************************************************************
	//
	//
	VlakRen* face0 = VLR_list[0]; 		// face0
	Material* face0mat = face0->mat; 	// face0mat
	string matname(face0mat->id.name); 	// matname
	// ***MULTIPLE SHADER***********************************************************************************************

	map<int, string>  MATlist;	// MATlist		 
	map<int, string>  FINALlist;	// FINALlist 
	FINALlist[0] = "none" ;	
	int indice = 0;			// indice
	int indiceFinal = 1;		// indiceFinal
	int new_mat = 0;		// switch
	std::string MATname;		// MATname

	// LOOP			MAKE MATlist : for face add material name in MATlist
	for (vector<VlakRen*>::const_iterator fci=VLR_list.begin(); fci!=VLR_list.end();++fci )
	{	
		VlakRen* vlr = *fci;
		MATname = vlr->mat->id.name;
		MATlist[indice] = MATname;
		indice += 1 ;
	}
	// LOOP			REMOVE DOUBLES : for name in MATlist 
	// for material in MATlist 
	for (map<int,string>::const_iterator fci=MATlist.begin(); fci!=MATlist.end();++fci )
	{
		new_mat = 0 ;
		// for material in FINALlist
		for ( map<int,string>::const_iterator final=FINALlist.begin(); final!=FINALlist.end();++final)
		{
			// if equal switch new_mat 
			if ( fci->second.compare(final->second) == 0  )  new_mat = 1;
			if ( fci->second.compare("none") == 0 ) printf("\n");
		}
		if ( new_mat == 0 )
		{
			FINALlist[indiceFinal] = fci->second;
			indiceFinal += 1 ;
			new_mat = 0;
		}
	}
	materials = FINALlist;
	indiceFinal = 1;
	// SHADERS
	// IF multi material
	if ( FINALlist.size() > 2 )
	{	
		if ( object )
		{
			sunflowMesh << "\tshaders" << " " <<  FINALlist.size() - 1 << endl ;
		}
		for ( map<int,string>::const_iterator final=FINALlist.begin(); final!=FINALlist.end();++final)
		{
			if ( object )
			{
				int gna;
				if ( final->second.compare("none") == 0 ) gna = 0 ;
				else sunflowMesh << "\t\t" << cleanName(final->second) << endl ;
			}
		}
	}
	// IF one material
	else 
	{
		if ( matname.length()==0 ) matname = "def";
		if ( object )
			sunflowMesh<<"\tshader " << cleanName(matname) << endl;
	}

	//
	// ***MODIFIERS**************************************************************************************************** 
	//
	int stop = 0;
	for (map<int, string>::const_iterator name=modifiers.begin();
			name!=modifiers.end(); ++name)
	{

		if ( stop == 1 ) break ;
		for ( map<int,string>::const_iterator material=FINALlist.begin();
				material!=FINALlist.end();++material)
		{
			std::string current;
			std::string list;
			current = name->second ;
			list = material->second + "_mod" ;
			cout << "current " << current <<endl;
			cout << "list" << list <<endl;

			if ( current.compare(list) == 0 )
			{
				sunflowMesh << "\tmodifier" << " " <<  cleanName(matname + "_mod") << endl;
				stop = 1;
				break;
			}
		}
	}
	// ***TRANSFORM****************************************************************************************************
	//--------------- have to apply matrix for meshlight
	
	if ( transform & object )
	sunflowMesh<< "\ttransform row" 	<< " " << obmat[0][0] << " " << obmat[1][0] << " " << obmat[2][0] << " " << obmat[3][0]
	       					<< " " << obmat[0][1] << " " << obmat[1][1] << " " << obmat[2][1] << " " << obmat[3][1]
					       	<< " " << obmat[0][2] << " " << obmat[1][2] << " " << obmat[2][2] << " " << obmat[3][2]
					        << " " << obmat[0][3] << " " << obmat[1][3] << " " << obmat[2][3] << " " << obmat[3][3] << endl ;
	// ***TYPE***
	if (object)
		sunflowMesh<<"\ttype generic-mesh"<<endl;
	// ***NAME***
	if (object)
		sunflowMesh<<"\tname" << " " << cleanName(obj->id.name)<<endl;
	
	//
	// VERTEX
	//
	//
	
	// inverse render matrix 	
	float mat[4][4];			// mat			matrix
	float imat[4][4];			// imat			inverse matrix
	MTC_Mat4MulMat4(mat, obj->obmat, re->viewmat);
	MTC_Mat4Invert(imat, mat);

	map<VertRen*, int> vert_idx;		// vert_idx     	for removing duplicate verts and creating an index list
	map<int, VertRen*>  SunflowVert;	// SunflowerVert	

	int vidx = 0;	//vidx	

	//
	// Do SunflowVert
	for (vector<VlakRen*>::const_iterator fci=VLR_list.begin();
				fci!=VLR_list.end();++fci)
	{
		VlakRen* vlr = *fci;
		VertRen* ver;
		float tvec[3];

		//Material* fmat = vlr->mat;
		// 1
		if (vert_idx.find(vlr->v1)==vert_idx.end())
	       {

			vert_idx[vlr->v1] = vidx++;
			SunflowVert[vidx-1] = vlr->v1;
			ver = vlr->v1;

			MTC_cp3Float(ver->co, tvec); //
			MTC_Mat4MulVecfl(imat, tvec); //
		}
		//2 
		if (vert_idx.find(vlr->v2)==vert_idx.end())
	       {
			vert_idx[vlr->v2] = vidx++;
			SunflowVert[vidx-1] = vlr->v2;
			ver = vlr->v2;
		}
		//3 
		if (vert_idx.find(vlr->v3)==vert_idx.end())
	       {
			vert_idx[vlr->v3] = vidx++;
			SunflowVert[vidx-1] = vlr->v3;
			ver = vlr->v3;
		}
		//4 

		if ((vlr->v4) && (vert_idx.find(vlr->v4)==vert_idx.end())) // !!!!!!!!!! 
	       {
			vert_idx[vlr->v4] = vidx++;
			SunflowVert[vidx-1] = vlr->v4;
			ver = vlr->v4;
		}
	}
	
	// WRITE
	sunflowMesh<<"\tpoints "<<vidx<<endl;
	//sunflowMesh<<"\tpoints "<<obr->totvert<<endl;   **** no : free vertices ***

	for (map<int,VertRen*>::const_iterator vect1=SunflowVert.begin();vect1!=SunflowVert.end();++vect1)
	{
		if (object)
		{
			float tvec[3];
			MTC_cp3Float(vect1->second->co, tvec);

			if ( transform == 0 )
			{
				float matx[4][4];
				MTC_Mat4MulMat4(matx, obj->obmat, re->viewmat);
				MTC_Mat4MulVecfl(imat, tvec);

				float matX,matY,matZ;

				matX = obmat[3][0];
				matY = obmat[3][1];
				matZ = obmat[3][2];

				sunflowMesh << "\t\t" 
					<< ( tvec[0] * obmat[0][0] ) + matX  << " " 
					<< ( tvec[1] * obmat[1][1] ) + matY  << " " 
					<< ( tvec[2] * obmat[2][2] ) + matZ  <<endl;
			}
			else
			{
				MTC_Mat4MulVecfl(imat, tvec);
				sunflowMesh << "\t\t" 
				<< tvec[0] << " "
				<< tvec[1] << " "
			       	<< tvec[2]<<endl;
			}
		}
		if (meshlight)
		{
			float tvec[3];
			MTC_cp3Float(vect1->second->co, tvec);
			//MTC_Mat4MulVecfl(imat, tvec);

			float matx[4][4];
			MTC_Mat4MulMat4(matx, obj->obmat, re->viewmat);
			//MTC_Mat4MulVecfl(matx, tvec);
			MTC_Mat4MulVecfl(imat, tvec);


		/*	
			float matX,matY,matZ;
			if ( obmat[3][0] == 0) matX = 1.0 ;
			else matX = obmat[3][0] ;
			if ( obmat[3][1] == 0) matY = 1.0 ;
			else matY = obmat[3][1] ;
			if ( obmat[3][2] == 0) matZ = 1.0 ;
			else matZ = obmat[3][2] ;

			sunflowMesh << "\t\t" 
				<< ( tvec[0] * obmat[0][0] )  + matX << " " 
				<< ( tvec[1] * obmat[1][1] ) + matY  << " " 
				<< ( tvec[2] * obmat[2][2] ) + matZ  <<endl;
		*/
		/*
			sunflowMesh << "\t\t" 
				<< matx[0][0] * tvec[0] + obmat[3][0] << " " 
				<< matx[1][1] * tvec[1] + obmat[3][1] << " " 
				<< matx[2][2] * tvec[2] + obmat[3][2] <<endl;
		*/
			sunflowMesh << "\t\t" 
				<< obmat[0][0] * (tvec[0] + obmat[3][0]) << " " 
				<< obmat[1][1] * (tvec[1] + obmat[3][1]) << " " 
				<< obmat[2][2] * (tvec[2] + obmat[3][2]) <<endl;
				
		}
	}
	// ***FACES/TRIANGLES**********************************************************************************************
	//
	//

	int numero = 0;
	
	//
	// COUNT	
	for (vector<VlakRen*>::const_iterator fci2=VLR_list.begin();
				fci2!=VLR_list.end();++fci2)
	{
		numero +=1;

		VlakRen* vlr = *fci2;
		if (vlr->v4) numero +=1;
	}
	//
	// WRITE

	sunflowMesh << "\ttriangles "<<numero<<"\n";

	for (vector<VlakRen*>::const_iterator fci2=VLR_list.begin();
				fci2!=VLR_list.end();++fci2)
	{
		VlakRen* vlr = *fci2;

		int idx1 = vert_idx.find(vlr->v1)->second;
		int idx2 = vert_idx.find(vlr->v2)->second;
		int idx3 = vert_idx.find(vlr->v3)->second;

		// WRITE
		sunflowMesh << "\t\t" << idx1 << " " << idx2 << " " << idx3 << endl;// a b c ++

		if (vlr->v4) {

			idx1 = vert_idx.find(vlr->v3)->second;
			idx2 = vert_idx.find(vlr->v4)->second;
			idx3 = vert_idx.find(vlr->v1)->second;

			sunflowMesh << "\t\t" << idx1 << " " << idx2 << " "  << idx3 << endl;// a b c --
		}
	}
	//
	// ***NORMALS******************************************************************************************************
	//
	
	bool no_auto = true;	//in case non-mesh, or mesh has no autosmooth
	bool smooth = false;

	if (obj->type==OB_MESH)
       	{
		Mesh* mesh = (Mesh*)obj->data;
		if (mesh->flag & ME_AUTOSMOOTH) {
			no_auto = false;
		}
	}
	// this for non-mesh as well
	if ( no_auto )
       	{
		// If AutoSmooth not used, since yafray currently cannot specify if a face is smooth
		// or flat shaded, the smooth flag of the first face is used to determine
		// the shading for the whole mesh
		if (face0->flag & ME_SMOOTH)
			smooth = true;
	}
	if ( smooth )
	{	

		if (object)
			sunflowMesh << "\tnormals vertex"<<endl;

		for (map<int,VertRen*>::const_iterator vect1=SunflowVert.begin();vect1!=SunflowVert.end();++vect1)
		{
			float tvec[3];
			MTC_cp3Float(vect1->second->n, tvec);
			//MTC_Mat4MulVecfl(imat, tvec);
			if (object)
				sunflowMesh << "\t\t" << tvec[0] << " " << tvec[1] << " " << tvec[2]<<endl;
		}
	}
	else
	{
		if (object)
			sunflowMesh << "\tnormals none" << endl;
	}
	
	//
	// ***UV***********************************************************************************************************
	//

	int uv_toggle;
	uv_toggle = 0;
	
	for (vector<VlakRen*>::const_iterator fci2=VLR_list.begin();
				fci2!=VLR_list.end();++fci2)
	{
		 
		VlakRen* vlr = *fci2;
		MTFace* uvc = RE_vlakren_get_tface(obr, vlr, obr->actmtface, NULL, 0); // possible uvcoords (v upside down)
		if ( uvc ) { uv_toggle = 1; break  ; }
	}
	if (object)
	{
		if ( uv_toggle == 0 )  sunflowMesh<<"\tuvs none"<<endl;
		if ( uv_toggle  == 1 ) sunflowMesh<<"\tuvs facevarying\n"<<endl;
	}
	// FOR UVs
	for (vector<VlakRen*>::const_iterator fci2=VLR_list.begin();
				fci2!=VLR_list.end();++fci2)
	{
		VlakRen* vlr = *fci2;
		MTFace* uvc = RE_vlakren_get_tface(obr, vlr, obr->actmtface, NULL, 0); // possible uvcoords (v upside down)

		if (uvc)
	       	{
			// QUAD 
			if (vlr->v4)
			{
				if (object)
				sunflowMesh
						<< "\t\t" << uvc->uv[0][0] << " " << uvc->uv[0][1] << endl // 1-uvc 
						<< "\t\t" << uvc->uv[1][0] << " " << uvc->uv[1][1] << endl 
						<< "\t\t" << uvc->uv[2][0] << " " << uvc->uv[2][1] << endl
						; 
				if (object )
				sunflowMesh	 << "\n"<<endl;
				if (object )
				sunflowMesh 

						<< "\t\t" << uvc->uv[2][0] << " " << uvc->uv[2][1] << endl 
						<< "\t\t" << uvc->uv[3][0] << " " << uvc->uv[3][1] << endl 
						<< "\t\t" << uvc->uv[0][0] << " " << uvc->uv[0][1] << endl
						;
				if (object)
				sunflowMesh 	<< "\n"<<endl;
			}
			// TRIS
			else
			{	
				if (object)
				sunflowMesh 	
						<< "\t\t" << uvc->uv[0][0] << " " << uvc->uv[0][1] << endl 
						<< "\t\t" << uvc->uv[1][0] << " " << uvc->uv[1][1] << endl 
						<< "\t\t" << uvc->uv[2][0] << " " << uvc->uv[2][1] << endl
						;
				if ( object )
				sunflowMesh 	<< "\n"<<endl;
			}
		}  
	}  
	//*** FACE INDICES ***********************************************************************************************

	std::string faceMAT;
	int FACEindice = -1 ; // cause none in list

	if ( (FINALlist.size() > 2) &  object )
	{
		sunflowMesh << "\tface_shaders" << endl ;

		// for FACE
		for (vector<VlakRen*>::const_iterator fci2=VLR_list.begin();
					fci2!=VLR_list.end();++fci2)
		{
			VlakRen* vlr = *fci2;
			faceMAT = vlr->mat->id.name;

			for ( map<int,string>::const_iterator final=FINALlist.begin(); final!=FINALlist.end();++final)
			{
				if ( faceMAT.compare(final->second) == 0 )
				{
					if ( object )
					{
						if( vlr->v4 )
						{
							sunflowMesh << "\t\t" << FACEindice << endl;
							sunflowMesh << "\t\t" << FACEindice << endl;
						}
						else
							sunflowMesh << "\t\t" << FACEindice << endl;
					}
				}
				else FACEindice += 1;
			}
			FACEindice = -1;
		}
	}
	// CLOSE }
	sunflowMesh<<"}"<<endl;
}
/* *********************************************************************************************************************************************	*/
/* LAMPS / CAMERA	************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/

/* WRITE WORLD *************************************************************************** */
bool SunflowFileRender_t::writeWorld()
{
	World *world = G.scene->world;

	if (world==NULL) return false;
	sunflowFile <<"background {" << endl;
	sunflowFile << "\tcolor" << " " << world->horr << " " << world->horg << " " << world->horb << endl;
	sunflowFile << "}" << endl;
	return true;
}
/* WRITE LAMPS *************************************************************************** */
void SunflowFileRender_t::writeLamps()
{
	printf("==WRITE LAMPS==\n");
	GroupObject *go;
	int i=0;
	
	// inverse viewmatrix needed for back2world transform
	float iview[4][4];
	// have to invert it here
	MTC_Mat4Invert(iview, re->viewmat);
	
	// all lamps
	for(go=(GroupObject *)re->lights.first; go; go= go->next, i++)
       	{
		LampRen* lamp = (LampRen *)go->lampren;
		
		// HEMI 
		if (lamp->type==LA_HEMI){ writeSun(lamp,iview); return;}

		// LAMP
		if (lamp->type==LA_LOCAL) 
		{
			sunflowFile << "light {"<<endl;
			sunflowFile<<"\ttype ";
			sunflowFile << "point" << endl;

			float lpco[3], lpvec[3];
			MTC_cp3Float(lamp->co, lpco);
			MTC_Mat4MulVecfl(iview, lpco);
			MTC_cp3Float(lamp->vec, lpvec);
			MTC_Mat4Mul3Vecfl(iview, lpvec);

			sunflowFile << "\tcolor { \"sRGB linear\" " << lamp->r << " "  << lamp->g << " "  << lamp->b <<" }"<< endl;//rgb
			sunflowFile << "\tpower" << " " << lamp->energy * 200  <<endl;
			sunflowFile << "\tp " << lpco[0] << " " << lpco[1] << " " << lpco[2] << endl;// x y z		
			sunflowFile << "}"<<endl;
		}
		// SUN 
		if (lamp->type==LA_SUN) 
		{
			float lpco[3], lpvec[3];
			MTC_cp3Float(lamp->co, lpco);
			MTC_Mat4MulVecfl(iview, lpco);
			MTC_cp3Float(lamp->vec, lpvec);
			MTC_Mat4Mul3Vecfl(iview, lpvec);

			sunflowFile << "light {"<<endl;
			sunflowFile<<"\ttype ";
			sunflowFile << "directional" << endl;

			sunflowFile << "\tsource " << lpco[0] << " " << lpco[1] << " " << lpco[2] << endl;// x y z		
			sunflowFile << "\ttarget " << lpco[0] + lpvec[0] << " " << lpco[1] + lpvec[1] << " " << lpco[2] + lpvec[2] << endl;// x y z		
			sunflowFile << "\tradius " << lamp->dist << endl;

			sunflowFile << "\temit { \"sRGB linear\" " << lamp->r << " "  << lamp->g << " "  << lamp->b <<" }"<< endl;//rgb
			sunflowFile << "\tintensity " << lamp->energy * 200 << endl;;
			sunflowFile << "}"<<endl;
		}
	}
}
/* WRITE SUN *************************************************************************** */
void SunflowFileRender_t::writeSun(LampRen* lamp,float iview[4][4])
{
		float lpco[3], lpvec[3];

		MTC_cp3Float(lamp->co, lpco);
		MTC_Mat4MulVecfl(iview, lpco);
		MTC_cp3Float(lamp->vec, lpvec);
		MTC_Mat4Mul3Vecfl(iview, lpvec);

		sunflowFile << "light {" << endl;
		sunflowFile << "\ttype sunsky" << endl;
		sunflowFile << "\tup 0 0 1" << endl;
		sunflowFile << "\teast 1 0 0 " <<endl;

		sunflowFile << "\tsundir" << " " <<  - lpvec[0] << " " <<  - lpvec[1] << " " <<  - lpvec[2] << endl;
		sunflowFile << "\tturbidity" << " " << lamp->SF_SUN_turbidity << endl;
		sunflowFile << "\tsamples" << " " << lamp->SF_SUN_samples << endl;
		sunflowFile << "}" << endl;

}
/* WRITE CAMERA *************************************************************************** */
void SunflowFileRender_t::writeCamera()
{
	float f_aspect = 1;
	float fdist = 1;

	if ((re->r.xsch*re->r.xasp)<=(re->r.ysch*re->r.yasp))   
		f_aspect = float(re->r.xsch*re->r.xasp)/float(re->r.ysch*re->r.yasp);

	Camera* cam=NULL;
	cam = (Camera*)maincam_obj->data;

	sunflowFile<<"camera {"<<endl;
	sunflowFile << "\ttype pinhole"<<endl;
	sunflowFile <<"\teye " <<maincam_obj->obmat[3][0]<<" "<<maincam_obj->obmat[3][1]<<" "<<maincam_obj->obmat[3][2]<<endl;
	sunflowFile <<"\ttarget"	<<" "<<maincam_obj->obmat[3][0] - fdist * re->viewmat[0][2]
					<<" "<<maincam_obj->obmat[3][1] - fdist * re->viewmat[1][2]
	      				<<" "<<maincam_obj->obmat[3][2] - fdist * re->viewmat[2][2] << endl;
	sunflowFile <<"\tup "<<re->viewmat[0][1]<<" "<<re->viewmat[1][1]<<" "<<re->viewmat[2][1]<<endl;
	sunflowFile << "\tfov "<<2.0 * (atan(1/(2.0*mainCamLens/(f_aspect*32.f))))*(180/M_PI)<<endl;
	sunflowFile <<"\taspect "<<float(re->r.xsch)/float(re->r.ysch)<<endl;

	// SHIFT
	float X;
	float Y;
	float x;
	float y;
	float FOV;

	//Y = y  * ( 2 * FOV/100 );
	x = cam->shiftx;
	y = cam->shifty;
	FOV = 2.0 * (atan(1/(2.0*mainCamLens/(f_aspect*32.f))))*(180/M_PI);
	X = x;
	Y = y  *  (2 * FOV/100);
	
	if ( X )
	{
		if ( Y ) 	sunflowFile << "\tshift" << " " << X  << " " << Y  << endl ;
		else  			sunflowFile << "\tshift" << " " << X << " " << "0" << endl ;
	}
	if ( Y )
	{
		if ( X ) printf("\n") ;
		else 			sunflowFile << "\tshift" << " " << "0" << " " << Y  << endl ;
	}
	// CLOSE
	sunflowFile << "}"<<endl;
}
/* *********************************************************************************************************************************************	*/
/* MATERIALS 		************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/
/* *********************************************************************************************************************************************	*/

/* WRITE MATERIALS AND MODULATORS	******************************************************************************************	*/
void SunflowFileRender_t::writeMaterialsAndModulators()
{
	for (map<string, Material*>::const_iterator blendmat=used_materials.begin();
			blendmat!=used_materials.end();++blendmat)
	{
		Material* matr = blendmat->second;

		cout << "==== WRITE SHADER====" << endl;
		writeShader(blendmat->first, matr);
	}
}
/* WRITE SHADERS	******************************************************************************************	*/
float sfR=1;
float sfG=1;
float sfB=1;
float delta = .25;
int counter = 1;

void SunflowFileRender_t::writeShader(	const std::string &shader_name,
	       				Material* matr,
				       	const std::string &facetexname)
{
	int mask = 0;
	int alpha = 0;
	int glass = 0;

	mask = G.scene->r.SF_MASK;
	alpha = G.scene->r.SF_MASK_alpha;
	if ( matr->SF_MAT_type == 4 ) glass =1;

	cout << "mask "<< mask << " alpha  " << alpha << " glass  " << glass << endl;
	cout << "material " << cleanName(shader_name) << endl;



	// USER
	if (matr->SF_MAT_type == 11 )
	{
		cout << "User Material" << endl;
	}
	// MASKS
	else if ( mask )
	{
		if ( (glass == 1) & (alpha == 1 ) ) 
		{
			string name = cleanName(shader_name);

			sunflowFile << "shader {" << endl;
			sunflowFile << "\tname"	<< " " << name  << endl;
			sunflowFile << "% MASK " << endl;
			sunflowFile << "\ttype glass" << endl;
			sunflowFile << "\teta " << "1" << endl;
			sunflowFile << "\tcolor" << " 1 1 1 " << endl;
			sunflowFile << "\tabsorption.distance" << " " << "100" << endl;
			sunflowFile << "\tabsorption.color" << " 1 1 1" << endl;
			sunflowFile << "}" << endl;
		}
		else
		{
			string name = cleanName(shader_name);
			std::stringstream color;

			cout << "..." << endl;

			sunflowFile << "shader {" << endl;
			sunflowFile << "\tname"	<< " " << name  << endl;
			sunflowFile << "\ttype constant" << endl;	

			if ( counter >= 6 ) counter = 1;
			if ( (sfR <= .1) & (delta <=.1 ) ) { sfR = .8 ; delta = .05 ; }
			if ( sfR <= 0 ) { sfR = .8 ; delta = .1 ; }
			if ( sfG <= 0 ) sfG = .8 ;
			if ( sfB <= 0 ) sfB = .8 ;

			if ( counter == 1 ) {sfR -= delta ; color << sfG ; color << " " ; color << sfR ; color << " " ; color << sfB ;}
			if ( counter == 2 ) {sfR -= delta ; color << sfB ; color << " " ; color << sfR ; color << " " ; color << sfG ;}
			if ( counter == 3 ) {sfG -= delta ; color << sfR ; color << " " ; color << sfG ; color << " " ; color << sfB ;}
			if ( counter == 4 ) {sfG -= delta ; color << sfB ; color << " " ; color << sfG ; color << " " ; color << sfR ;}
			if ( counter == 5 ) {sfB -= delta ; color << sfR ; color << " " ; color << sfB ; color << " " ; color << sfG ;}
			if ( counter == 6 ) {sfB -= delta ; color << sfG ; color << " " ; color << sfB ; color << " " ; color << sfR ;}

			sunflowFile << "\tcolor" << " " << color.str() << endl;
			counter++;
			sunflowFile << "}" << endl;
		}
	}
	// REGULAR
	else
	{
		int texture = 0 ;
		std::stringstream texture_path;
		int material = matr->SF_MAT_type;

		if ( matr->SF_MAT_use_texture ) texture = 1;
		// TEXTURE
		if ( matr->mtex[0] ) /// CRASH !! empty slot
		{
			cout << "//TEXTURE" << endl;
			MTex* Mtexture = matr->mtex[0];
			Tex* mat_texture = Mtexture->tex;
			Image* image = mat_texture->ima;
			texture_path << "\ttexture" << " " << image->name << endl;
			texture = 1;
		}
		// MOFIFIER
		else if ( matr->mtex[1])
		{
			cout << "//MODIFIER" << endl;
			MTex* Mtexture = matr->mtex[1];
			Tex* texture = Mtexture->tex;
			Image* image = texture->ima;
			texture_path << "\ttexture" << " " << image->name << endl;

			modifiers[indice] = cleanName(shader_name + "_mod"); 
			textures[indice] = texture_path.str();

			indice++;

			sunflowFile << "modifier {" << endl;
			sunflowFile << "\tname" << " " << cleanName(shader_name + "_mod") << endl;
			sunflowFile << "\ttype" << " " << "bump" << endl;
			sunflowFile << "\ttexture" << " " << image->name <<endl;
			sunflowFile << "\tscale" << " " << (matr->mtex[1]->norfac) * 100  << endl;
			sunflowFile << "}" << endl;
		}
		else
		{
			texture = 0;
		}
		texture = - texture ; // :)


		cout << "texture " << texture << " material " << material << endl;
		// NAME
		sunflowFile << "shader {"<<endl;
		sunflowFile << "\tname" 	<< " " << cleanName(shader_name) <<endl;

		// DIFFUSE
		if ( material == 0 )
		{
			sunflowFile << "\ttype diffuse" << endl;
			if ( texture )
			{
				sunflowFile << texture_path.str() << endl;
			}
			else
			{
				sunflowFile << "\tdiff" << " " << matr->r << " " << matr->g << " " << matr->b << " " << endl;
			}
		}
		// PHONG
		if ( material == 1 )
		{
			sunflowFile << "\ttype phong" << endl;
			if ( texture )
				sunflowFile << texture_path.str() ;
			else
			{
				sunflowFile << "\tdiff" << " " << matr->r << " " << matr->g << " " << matr->b << " " << endl;
			}
			sunflowFile << "\tspec" <<
			       " " << matr->specr * matr->spec <<
			       " " << matr->specg * matr->spec <<
			       " " << matr->specb * matr->spec <<
			       " " << matr->har << endl;// spec power ?
		}
		// SHINY 
		if ( material == 2 )
		{
			sunflowFile << "\ttype shiny" << endl;
			if ( texture )
				sunflowFile << texture_path.str() ;
			else
			{
				sunflowFile << "\tdiff" << " " << matr->r << " " << matr->g << " " << matr->b << endl;
			}
			sunflowFile << "\trefl" << " " << matr->ray_mirror << endl;
		}
		// WARD 
		if ( material == 3 )
		{
			sunflowFile << "\ttype ward" << endl;
			if ( texture )
				sunflowFile << texture_path.str() ;
			else
			{
				sunflowFile << "\tdiff" << " " << matr->r << " " << matr->g << " " << matr->b << " " << endl;
			}
			sunflowFile << "\tspec" <<
			       " " << matr->specr * matr->spec <<
			       " " << matr->specg * matr->spec <<
			       " " << matr->specb * matr->spec <<
			       endl;

			sunflowFile << "\trough" << " " << matr->gloss_mir << " " << matr->adapt_thresh_mir << endl;
			sunflowFile << "\tsamples" << " " << matr->SF_MAT_samples  << endl;
		}
		// GLASS 
		if ( material == 4 )
		{
			sunflowFile << "\ttype glass" << endl;
			sunflowFile << "\teta" << " " << matr->ang << endl;
			sunflowFile << "\tcolor" <<
			       " " << matr->specr * matr->r <<
			       " " << matr->specg * matr->g <<
			       " " << matr->specb * matr->g <<
				endl;

			sunflowFile << "\tabsorption.distance" << " " << matr->alpha * 10.0 << endl;
			sunflowFile << "\tabsorption.color { \"sRGB nonlinear\" " <<
			       " " << matr->specr * matr->mirr <<
			       " " << matr->specg * matr->mirg <<
			       " " << matr->specb * matr->mirg <<
			       " }"<< 
				endl;
		}
		// MIRROR 
		if ( material == 5 )
		{
			sunflowFile << "\ttype mirror" << endl;
			sunflowFile << "\trefl" <<
			       " " << matr->specr * matr->r <<
			       " " << matr->specg * matr->g <<
			       " " << matr->specb * matr->g <<
				endl;
		}
		// CONSTANT 
		if ( material == 6 )
		{
			sunflowFile << "\ttype constant" << endl;
			sunflowFile << "\tcolor" <<
			       " " << matr->specr * matr->r <<
			       " " << matr->specg * matr->g <<
			       " " << matr->specb * matr->g <<
				endl;
		}

	// CLOSE }
	sunflowFile << "}"<<endl;
	}
}



