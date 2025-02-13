/****************************************************************************
* MeshLab                                                           o o     *
* Visual and Computer Graphics Library                            o     o   *
*                                                                _   O  _   *
* Copyright(C) 2004-2021                                           \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/

#include <QString>
#include <QtGlobal>
#include <QFileInfo>

#include "mesh_model.h"
#include "../utilities/load_save.h"

#include <wrap/gl/math.h>

#include <QDir>
#include <utility>

using namespace vcg;

MeshModel::MeshModel(int id, const QString& fullFileName, const QString& labelName) :
	visible(true)
{
	/*glw.m = &(cm);*/
	clear();
	_id=id;
	if(!fullFileName.isEmpty())   this->fullPathFileName=fullFileName;
	if(!labelName.isEmpty())	 this->_label=labelName;
}

void MeshModel::clear()
{
	setMeshModified(false);
	// These data are always active on the mesh
	currentDataMask = MM_NONE;
	currentDataMask |= MM_VERTCOORD | MM_VERTNORMAL | MM_VERTFLAG ;
	currentDataMask |= MM_FACEVERT  | MM_FACENORMAL | MM_FACEFLAG ;

	visible=true;
	cm.Tr.SetIdentity();
	cm.sfn=0;
	cm.svn=0;
}

void MeshModel::updateBoxAndNormals()
{
	tri::UpdateBounding<CMeshO>::Box(cm);
	if(cm.fn>0) {
		tri::UpdateNormal<CMeshO>::PerFaceNormalized(cm);
		tri::UpdateNormal<CMeshO>::PerVertexAngleWeighted(cm);
	}
}

QString MeshModel::relativePathName(const QString& path) const
{
	QDir documentDir (path);
	QString relPath=documentDir.relativeFilePath(this->fullPathFileName);

	//if(relPath.size()>1 && relPath[0]=='.' &&  relPath[1]=='.')
	//	qDebug("Error we have a mesh that is not in the same folder of the project: %s ", qUtf8Printable(relPath));

	return relPath;
}

/**
 * @brief Starting from the (still unloaded) textures contained in the contained
 * CMeshO, loads the textures in the map of QImages contained in the MeshModel.
 *
 * The contained CMeshO will have a list of texture names like "filename.png",
 * and these names will be mapped with the actual loaded image in the map
 * "textures".
 *
 * When a texture is not found, a dummy texture will be used (":/img/dummy.png").
 *
 * Returns the list of non-loaded textures that have been modified with
 * ":/img/dummy.png" in the contained mesh.
 */
std::list<std::string> MeshModel::loadTextures(
		GLLogStream* log,
		vcg::CallBackPos* cb)
{
	std::list<std::string> unloadedTextures;
	for (std::string& textName : cm.textures){
		if (textures.find(textName) == textures.end()){
			QImage img(":/img/dummy.png");
			QFileInfo finfo(QString::fromStdString(textName));
			try {
				img = meshlab::loadImage(finfo.absoluteFilePath(), log, cb);
				textName = finfo.fileName().toStdString();
			} catch (const MLException& e) {
				try { //could be relative to the meshmodel
					QFileInfo mfi(fullName());
					QString fn2 = mfi.absolutePath() + "/" + finfo.filePath();
					img = meshlab::loadImage(fn2, log, cb);
					textName = finfo.filePath().toStdString();
				} catch (const MLException& e) {
					if (log){
						log->log(
							GLLogStream::WARNING, "Failed loading " + textName +
							"; using a dummy texture");
					}
					else {
						std::cerr <<
							"Failed loading " + textName + "; using a dummy texture\n";
					}
					unloadedTextures.push_back(textName);
					textName = "dummy.png";
				}
			}
			textures[textName] = img;
		}
	}
	return unloadedTextures;
}

void MeshModel::saveTextures(
		const QString& basePath,
		int quality,
		GLLogStream* log,
		CallBackPos* cb)
{
	for (const std::string& tname : cm.textures){
		meshlab::saveImage(
				basePath + "/" + QString::fromStdString(tname),
				textures.at(tname), quality, log, cb);
	}
}

QImage MeshModel::getTexture(const std::string& tn) const
{
	auto it = textures.find(tn);
	if (it != textures.end())
		return it->second;
	else
		return QImage();
}

const std::map<std::string, QImage>& MeshModel::getTextures() const
{
	return textures;
}

void MeshModel::clearTextures()
{
	textures.clear();
	cm.textures.clear();
}

void MeshModel::addTexture(std::string name, const QImage& txt)
{
	if (textures.find(name) == textures.end()){
		// just to be sure to not make duplicates in the contained mesh list of textures
		if (std::find(cm.textures.begin(), cm.textures.end(), name) == cm.textures.end())
			cm.textures.push_back(name);
		textures[name]=txt;
	}
}

void MeshModel::setTexture(std::string name, const QImage& txt)
{
	auto it = textures.find(name);
	if (it != textures.end())
		it->second = txt;
}

void MeshModel::changeTextureName(
		const std::string& oldName,
		std::string newName)
{
	if (oldName != newName) {
		auto mit = textures.find(oldName);
		auto tit = std::find(cm.textures.begin(), cm.textures.end(), oldName);
		if (mit != textures.end() && tit != cm.textures.end()){
			*tit = newName;

			textures[newName] = mit->second;
			textures.erase(mit);
		}
	}
}

int MeshModel::io2mm(int single_iobit)
{
	switch(single_iobit)
	{
	case tri::io::Mask::IOM_NONE         : return  MM_NONE;
	case tri::io::Mask::IOM_VERTCOORD    : return  MM_VERTCOORD;
	case tri::io::Mask::IOM_VERTCOLOR    : return  MM_VERTCOLOR;
	case tri::io::Mask::IOM_VERTFLAGS    : return  MM_VERTFLAG;
	case tri::io::Mask::IOM_VERTQUALITY  : return  MM_VERTQUALITY;
	case tri::io::Mask::IOM_VERTNORMAL   : return  MM_VERTNORMAL;
	case tri::io::Mask::IOM_VERTTEXCOORD : return  MM_VERTTEXCOORD;
	case tri::io::Mask::IOM_VERTRADIUS   : return  MM_VERTRADIUS;

	case tri::io::Mask::IOM_FACEINDEX    : return  MM_FACEVERT  ;
	case tri::io::Mask::IOM_FACEFLAGS    : return  MM_FACEFLAG  ;
	case tri::io::Mask::IOM_FACECOLOR    : return  MM_FACECOLOR  ;
	case tri::io::Mask::IOM_FACEQUALITY  : return  MM_FACEQUALITY;
	case tri::io::Mask::IOM_FACENORMAL   : return  MM_FACENORMAL ;

	case tri::io::Mask::IOM_WEDGTEXCOORD : return  MM_WEDGTEXCOORD;
	case tri::io::Mask::IOM_WEDGCOLOR    : return  MM_WEDGCOLOR;
	case tri::io::Mask::IOM_WEDGNORMAL   : return  MM_WEDGNORMAL  ;

	case tri::io::Mask::IOM_BITPOLYGONAL : return  MM_POLYGONAL  ;

	default:
		assert(0);
		return MM_NONE;  // FIXME: Returning this is not the best solution (!)
		break;
	} ;
}


/**** DATAMASK STUFF ****/

bool MeshModel::hasDataMask(const int maskToBeTested) const
{
	return ((currentDataMask & maskToBeTested)!= 0);
}

bool MeshModel::hasPerVertexColor() const
{
	return currentDataMask & MM_VERTCOLOR;
}

bool MeshModel::hasPerVertexQuality() const
{
	return currentDataMask & MM_VERTQUALITY;
}

bool MeshModel::hasPerVertexTexCoord() const
{
	return currentDataMask & MM_VERTTEXCOORD;
}

bool MeshModel::hasPerFaceColor() const
{
	return currentDataMask & MM_FACECOLOR;
}

bool MeshModel::hasPerFaceQuality() const
{
	return currentDataMask & MM_FACEQUALITY;
}

bool MeshModel::hasPerFaceWedgeTexCoords() const
{
	return currentDataMask & MM_WEDGTEXCOORD;
}

void MeshModel::updateDataMask()
{
	currentDataMask = MM_NONE;
	currentDataMask |=
			MM_VERTCOORD | MM_VERTNORMAL | MM_VERTFLAG |
			MM_VERTQUALITY | MM_VERTCOLOR;
	currentDataMask |=
			MM_FACEVERT  | MM_FACENORMAL | MM_FACEFLAG ;
	if (cm.vert.IsVFAdjacencyEnabled())
		currentDataMask |= MM_VERTFACETOPO;
	if (cm.vert.IsMarkEnabled())
		currentDataMask |= MM_VERTMARK;
	if (cm.vert.IsTexCoordEnabled())
		currentDataMask |= MM_VERTTEXCOORD;
	if (cm.vert.IsCurvatureDirEnabled())
		currentDataMask |= MM_VERTCURVDIR;
	if (cm.vert.IsRadiusEnabled())
		currentDataMask |= MM_VERTRADIUS;
	if (cm.face.IsQualityEnabled())
		currentDataMask |= MM_FACEQUALITY;
	if (cm.face.IsMarkEnabled())
		currentDataMask |= MM_FACEMARK;
	if (cm.face.IsColorEnabled())
		currentDataMask |= MM_FACECOLOR;
	if (cm.face.IsFFAdjacencyEnabled())
		currentDataMask |= MM_FACEFACETOPO;
	if (cm.face.IsVFAdjacencyEnabled())
		currentDataMask |= MM_VERTFACETOPO;
	if (cm.face.IsCurvatureDirEnabled())
		currentDataMask |= MM_FACECURVDIR;
	if (cm.face.IsWedgeTexCoordEnabled())
		currentDataMask |= MM_WEDGTEXCOORD;
}

void MeshModel::updateDataMask(const MeshModel *m)
{
	updateDataMask(m->currentDataMask);
}

void MeshModel::updateDataMask(int neededDataMask)
{
	if((neededDataMask & MM_FACEFACETOPO)!=0)
	{
		cm.face.EnableFFAdjacency();
		tri::UpdateTopology<CMeshO>::FaceFace(cm);
	}
	if((neededDataMask & MM_VERTFACETOPO)!=0)
	{
		cm.vert.EnableVFAdjacency();
		cm.face.EnableVFAdjacency();
		tri::UpdateTopology<CMeshO>::VertexFace(cm);
	}

	if((neededDataMask & MM_WEDGTEXCOORD)!=0)
		cm.face.EnableWedgeTexCoord();
	if((neededDataMask & MM_FACECOLOR)!=0)
		cm.face.EnableColor();
	if((neededDataMask & MM_FACEQUALITY)!=0)
		cm.face.EnableQuality();
	if((neededDataMask & MM_FACECURVDIR)!=0)
		cm.face.EnableCurvatureDir();
	if((neededDataMask & MM_FACEMARK)!=0)
		cm.face.EnableMark();
	if((neededDataMask & MM_VERTMARK)!=0)
		cm.vert.EnableMark();
	if((neededDataMask & MM_VERTCURVDIR)!=0)
		cm.vert.EnableCurvatureDir();
	if((neededDataMask & MM_VERTRADIUS)!=0)
		cm.vert.EnableRadius();
	if((neededDataMask & MM_VERTTEXCOORD)!=0)
		cm.vert.EnableTexCoord();

	currentDataMask |= neededDataMask;
}

void MeshModel::clearDataMask(int unneededDataMask)
{
	if( ( (unneededDataMask & MM_VERTFACETOPO)!=0)	&& hasDataMask(MM_VERTFACETOPO)) {cm.face.DisableVFAdjacency();
	cm.vert.DisableVFAdjacency(); }
	if( ( (unneededDataMask & MM_FACEFACETOPO)!=0)	&& hasDataMask(MM_FACEFACETOPO))	cm.face.DisableFFAdjacency();

	if( ( (unneededDataMask & MM_WEDGTEXCOORD)!=0)	&& hasDataMask(MM_WEDGTEXCOORD)) 	cm.face.DisableWedgeTexCoord();
	if( ( (unneededDataMask & MM_FACECOLOR)!=0)			&& hasDataMask(MM_FACECOLOR))			cm.face.DisableColor();
	if( ( (unneededDataMask & MM_FACEQUALITY)!=0)		&& hasDataMask(MM_FACEQUALITY))		cm.face.DisableQuality();
	if( ( (unneededDataMask & MM_FACEMARK)!=0)			&& hasDataMask(MM_FACEMARK))			cm.face.DisableMark();
	if( ( (unneededDataMask & MM_VERTMARK)!=0)			&& hasDataMask(MM_VERTMARK))			cm.vert.DisableMark();
	if( ( (unneededDataMask & MM_VERTCURV)!=0)			&& hasDataMask(MM_VERTCURV))			cm.vert.DisableCurvature();
	if( ( (unneededDataMask & MM_VERTCURVDIR)!=0)		&& hasDataMask(MM_VERTCURVDIR))		cm.vert.DisableCurvatureDir();
	if( ( (unneededDataMask & MM_VERTRADIUS)!=0)		&& hasDataMask(MM_VERTRADIUS))		cm.vert.DisableRadius();
	if( ( (unneededDataMask & MM_VERTTEXCOORD)!=0)	&& hasDataMask(MM_VERTTEXCOORD))	cm.vert.DisableTexCoord();

	currentDataMask = currentDataMask & (~unneededDataMask);
}

void MeshModel::enable(int openingFileMask)
{
	if( openingFileMask & tri::io::Mask::IOM_VERTTEXCOORD )
		updateDataMask(MM_VERTTEXCOORD);
	if( openingFileMask & tri::io::Mask::IOM_WEDGTEXCOORD )
		updateDataMask(MM_WEDGTEXCOORD);
	if( openingFileMask & tri::io::Mask::IOM_VERTCOLOR	)
		updateDataMask(MM_VERTCOLOR);
	if( openingFileMask & tri::io::Mask::IOM_FACECOLOR	)
		updateDataMask(MM_FACECOLOR);
	if( openingFileMask & tri::io::Mask::IOM_VERTRADIUS   )
		updateDataMask(MM_VERTRADIUS);
	if( openingFileMask & tri::io::Mask::IOM_CAMERA	   )
		updateDataMask(MM_CAMERA);
	if( openingFileMask & tri::io::Mask::IOM_VERTQUALITY  )
		updateDataMask(MM_VERTQUALITY);
	if( openingFileMask & tri::io::Mask::IOM_FACEQUALITY  )
		updateDataMask(MM_FACEQUALITY);
	if( openingFileMask & tri::io::Mask::IOM_BITPOLYGONAL )
		updateDataMask(MM_POLYGONAL);
}

bool MeshModel::meshModified() const
{
	return modified;
}

void MeshModel::setMeshModified(bool b)
{
	modified = b;
}

int MeshModel::dataMask() const
{
	return currentDataMask;
}
