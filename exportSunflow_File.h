#ifndef __EXPORTSUNFLOW_FILE_H
#define __EXPORTSUNFLOW_FILE_H

#include"yafray_Render.h"
#include "DNA_text_types.h"

class SunflowFileRender_t : public yafrayRender_t
{
	public:
		virtual ~SunflowFileRender_t() {}

	protected:

		virtual void writeTextures();
		virtual void writeShader(const std::string &shader_name, Material* matr, const std::string &facetexname="");
		virtual void writeMaterialsAndModulators();
		virtual void writeObject(Object*, ObjectRen*, const std::vector<VlakRen*, std::allocator<VlakRen*> >&, const float (*)[4]);
		virtual void writeAllObjects();
		virtual void writeLamps();
		virtual void writeCamera();
		virtual void writeAreaLamp(LampRen*, int, float (*)[4]);
		virtual void writeHemilight();
		virtual void writePathlight();
		virtual bool writeWorld();
		virtual bool writeRender();
		virtual bool initExport();
		virtual bool finishExport();

		virtual void writeObjectSunflow( Object*, ObjectRen*, const std::vector<VlakRen*, std::allocator<VlakRen*> >&, const float (*)[4]);
		virtual void getUserText(Text* Text_sunflow);
		virtual bool findText();
		virtual void writeSun(LampRen* ,float [4][4]);

		void displayImage();
		bool executeYafray(const std::string &xmlpath);

		Text *Text_sunflow;

		std::string texturePath;
		std::string userFile;
		std::string xmlpath;
		std::string export_scene;
		std::string export_geo;
		std::string export_image;
		std::string current_material;
		std::ofstream sunflowFile;
		std::ofstream sunflowMesh;

		int transform;
	
};

#endif
