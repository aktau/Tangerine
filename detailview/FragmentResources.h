#ifndef FRAGMENTRESOURCES_H_
#define FRAGMENTRESOURCES_H_

#include <QDebug>
#include <QString>
#include <QSet>

#include "CMesh.h"
#include "Database.h"

// TODO: load and unload colors
struct FragmentResources {
	QString id;

	thera::Fragment::meshEnum activeMesh;
	QSet<thera::Fragment::meshEnum> loadedMeshes;

	std::vector<Color> colors;

	FragmentResources(const QString& _id) : id(_id) { }
	FragmentResources(const QString& _id, thera::Fragment::meshEnum requestedMesh, bool needColors = true) : id(_id) { load(requestedMesh, needColors); }

	// will unload all currently loaded data that is not required
	// will not reload data that is already loaded
	// changes the active mesh
	bool loadOnly(thera::Fragment::meshEnum requestedMesh, bool needColors = true) {
		if (load(requestedMesh, needColors)) {
			const thera::Fragment *fragment = thera::Database::fragment(id);

			// unload the ones that weren' requested
			foreach (thera::Fragment::meshEnum loadedMesh, loadedMeshes) {
				if (loadedMesh != requestedMesh) {
					//qDebug() << "FragmentResources::~loadOnly: unloaded/unpinned" << id << "//" << loadedMesh;

					fragment->mesh(loadedMesh).unpin();

					loadedMeshes.remove(loadedMesh);
				}
			}

			return true;
		}
		else {
			return false;
		}
	}

	inline bool load(thera::Fragment::meshEnum requestedMesh, bool needColors = true) {
		const thera::Fragment *fragment = thera::Database::fragment(id);

		if (!loadedMeshes.contains(requestedMesh)) {
			if (!fragment) {
				//qDebug() << "FragmentResources::load: fragment" << id << "not found!";

				return false;
			}
			else {
				if (!fragment->mesh(requestedMesh).pin()) {
					//qDebug() << "FragmentResources::load: pinning mesh" << requestedMesh << "of fragment" << id << "failed";

					return false;
				}
				else {
					thera::Mesh *m = &*fragment->mesh(requestedMesh);

					m->need_normals();
					m->need_tstrips();
					m->need_bsphere();

					loadedMeshes << requestedMesh;
				}
			}
		}

		activeMesh = requestedMesh;

		if (needColors && requestedMesh == thera::Fragment::HIRES_MESH && colors.empty()) {
			loadColors(fragment, requestedMesh);
		}

		return true;
	}

	~FragmentResources() {
		// use the Database approach to retrieving fragments
		// it's slightly more expensive than getting it via
		// the TabletopModel (2 map lookups instead of 1)
		// but the advantage is we don't need to store a
		// TabletopModel

		//qDebug() << "Fragment resources for" << id << "are about to be destroyed";

		const thera::Fragment *fragment = thera::Database::fragment(id);

		if (fragment) {
			foreach (thera::Fragment::meshEnum type, loadedMeshes) {
				//qDebug() << "FragmentResources::~FragmentResources: unloaded/unpinned" << id << "//" << type;

				fragment->mesh(type).unpin();
			}
		}
		else {
			qDebug() << "~FragmentResources: fragment" << id << "didn't even exist in the database, removing without unpinning";
		}

		// the colordata is automatically removed because the color vector is on the stack
		//loadedMeshes.clear(); <--- will happend automatically
	}

	private:
		FragmentResources(const FragmentResources&);
		FragmentResources& operator=(const FragmentResources&);

		inline void loadColors(const thera::Fragment *fragment, thera::Fragment::meshEnum requestedMesh) {
			using namespace thera;

			Mesh *mesh = &*fragment->mesh(requestedMesh);
			const MMImage *mmimg = fragment->color(Fragment::FRONT);

			if (mmimg && mesh->colors.size()) {
				const CImage &cimg = mmimg->fetchImageFromLevel(0);
				const CImage &cmask = fragment->masks(Fragment::FRONT);

				// pin now and unpin automatically when going out of scope
				AutoPin p1(cimg);
				AutoPin p2(cmask);

				Image *img = &(*cimg);
				Image *mask = &(*cmask);

				colors = mesh->colors;
				std::vector<Color> &c = colors;

				for (size_t i = 0, ii =  mesh->vertices.size(); i < ii; ++i) {
					vec &v = mesh->vertices[i];
					//vec &n = m->normals[i];  // not used

					// if (n[2] < 0) continue; // only do up-pointing vertices
					if (v[2] < -2) continue;

					vec4 mc = mask->bilinMM(v[0], v[1]);

					if (mc[Fragment::FMASK] != 1) continue;
					vec4 nc = img->bilinMM(v[0], v[1]);

					c[i] = Color(nc[0], nc[1], nc[2]);
				}
			}
		}
};

#endif /* FRAGMENTRESOURCES_H_ */
