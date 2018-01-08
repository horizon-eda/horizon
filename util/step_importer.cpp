#include "step_importer.hpp"
#include <TDocStd_Document.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <XCAFApp_Application.hxx>
#include <Handle_XCAFApp_Application.hxx>

#include <AIS_Shape.hxx>

#include <IGESControl_Reader.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <Interface_Static.hxx>

#include <STEPControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>

#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <Handle_XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>

#include <Quantity_Color.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Precision.hxx>

#include <TDF_LabelSequence.hxx>
#include <TDF_ChildIterator.hxx>

#define USER_PREC (0.14)
#define USER_ANGLE (0.52359878)



namespace STEPImporter {

	//adapted from https://github.com/KiCad/kicad-source-mirror/blob/master/plugins/3d/oce/loadmodel.cpp
	struct DATA;

	bool processNode( const TopoDS_Shape& shape, DATA& data );

	bool processComp( const TopoDS_Shape& shape, DATA& data);

	bool processFace( const TopoDS_Face& face, DATA& data);

	struct DATA
	{
		Handle( TDocStd_Document ) m_doc;
		Handle( XCAFDoc_ColorTool ) m_color;
		Handle( XCAFDoc_ShapeTool ) m_assy;
		bool hasSolid;

		std::deque<Face> *faces;

	};


	bool readSTEP( Handle(TDocStd_Document)& m_doc, const char* fname )
	{
		STEPCAFControl_Reader reader;
		IFSelect_ReturnStatus stat  = reader.ReadFile( fname );

		if( stat != IFSelect_RetDone )
			return false;

		// Enable user-defined shape precision
		if( !Interface_Static::SetIVal( "read.precision.mode", 1 ) )
			return false;

		// Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
		if( !Interface_Static::SetRVal( "read.precision.val", USER_PREC ) )
			return false;

		// set other translation options
		reader.SetColorMode(true);  // use model colors
		reader.SetNameMode(false);  // don't use label names
		reader.SetLayerMode(false); // ignore LAYER data

		if ( !reader.Transfer( m_doc ) )
		{
			m_doc->Close();
			return false;
		}

		// are there any shapes to translate?
		if( reader.NbRootsForTransfer() < 1 )
			return false;

		return true;
	}






	bool getColor( DATA& data, TDF_Label label, Quantity_Color& color )
	{
		while( true )
		{
			if( data.m_color->GetColor( label, XCAFDoc_ColorGen, color ) )
				return true;
			else if( data.m_color->GetColor( label, XCAFDoc_ColorSurf, color ) )
				return true;
			else if( data.m_color->GetColor( label, XCAFDoc_ColorCurv, color ) )
				return true;

			label = label.Father();

			if( label.IsNull() )
				break;
		};

		return false;
	}


	bool processFace( const TopoDS_Face& face, DATA& data, Quantity_Color* color )
	{
		if( Standard_True == face.IsNull() )
			return false;

		//bool reverse = ( face.Orientation() == TopAbs_REVERSED );


	//	bool useBothSides = false;


		TopLoc_Location loc;
		Standard_Boolean isTessellate (Standard_False);
		Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation( face, loc );

		if( triangulation.IsNull() || triangulation->Deflection() > USER_PREC + Precision::Confusion() )
			isTessellate = Standard_True;

		if (isTessellate)
		{
			BRepMesh_IncrementalMesh IM(face, USER_PREC, Standard_False, USER_ANGLE );
			triangulation = BRep_Tool::Triangulation( face, loc );
		}

		if( triangulation.IsNull() == Standard_True )
			return false;

		Quantity_Color lcolor;

		// check for a face color; this has precedence over SOLID colors
		do
		{
			TDF_Label L;

			if( data.m_color->ShapeTool()->Search( face, L ) )
			{
				if( data.m_color->GetColor( L, XCAFDoc_ColorGen, lcolor )
					|| data.m_color->GetColor( L, XCAFDoc_ColorCurv, lcolor )
					|| data.m_color->GetColor( L, XCAFDoc_ColorSurf, lcolor ) )
					color = &lcolor;
			}
		} while( 0 );

		const TColgp_Array1OfPnt&    arrPolyNodes = triangulation->Nodes();
		const Poly_Array1OfTriangle& arrTriangles = triangulation->Triangles();

		data.faces->emplace_back();
		auto &face_out = data.faces->back();
		face_out.color.r = color->Red();
		face_out.color.g = color->Green();
		face_out.color.b = color->Blue();
		face_out.vertices.reserve(triangulation->NbNodes());

		for(int i = 1; i <= triangulation->NbNodes(); i++)
		{
			gp_XYZ v( arrPolyNodes(i).Coord() );
			face_out.vertices.emplace_back(v.X(), v.Y(), v.Z());
		   // std::cout <<  v.X() << " " <<  v.Y() << " " << v.Z() << std::endl;
		  //  vertices.push_back( SGPOINT( v.X(), v.Y(), v.Z() ) );
		}

		face_out.triangle_indices.reserve(triangulation->NbTriangles());
		for(int i = 1; i <= triangulation->NbTriangles(); i++)
		{
			int a, b, c;
			arrTriangles( i ).Get( a, b, c );
			face_out.triangle_indices.emplace_back(a-1,b-1,c-1);
		}


		return true;
	}


	bool processShell( const TopoDS_Shape& shape, DATA& data, Quantity_Color* color )
	{
		TopoDS_Iterator it;
		bool ret = false;

		for( it.Initialize( shape, false, false ); it.More(); it.Next() )
		{
			const TopoDS_Face& face = TopoDS::Face( it.Value() );

		   if( processFace( face, data, color ) )
				ret = true;
		}

		return ret;
	}

	bool processSolid( const TopoDS_Shape& shape, DATA& data)
	{
		TDF_Label label = data.m_assy->FindShape( shape, Standard_False );
		   bool ret = false;

		data.hasSolid = true;

		Quantity_Color col;
		Quantity_Color* lcolor = NULL;

		if( label.IsNull() )
		{

		}
		else
		{


			if( getColor( data, label, col ) )
				lcolor = &col;
		}

		TopoDS_Iterator it;
		TopLoc_Location loc = shape.Location();

		for( it.Initialize( shape, false, false ); it.More(); it.Next() )
		{
			const TopoDS_Shape& subShape = it.Value();

		   if( processShell( subShape, data, lcolor ) )
			 ret = true;
		}

		return ret;
	}


	bool processComp( const TopoDS_Shape& shape, DATA& data)
	{
		TopoDS_Iterator it;
		TopLoc_Location loc = shape.Location();
		bool ret = false;


		for( it.Initialize( shape, false, false ); it.More(); it.Next() )
		{
			const TopoDS_Shape& subShape = it.Value();
			TopAbs_ShapeEnum stype = subShape.ShapeType();
			data.hasSolid = false;

			switch( stype )
			{
				case TopAbs_COMPOUND:
				case TopAbs_COMPSOLID:
					if( processComp( subShape, data) )
						ret = true;
					break;

				case TopAbs_SOLID:
					if( processSolid( subShape, data ) )
						ret = true;
					break;

				case TopAbs_SHELL:
					if( processShell( subShape, data, NULL ) )
						ret = true;
					break;

				case TopAbs_FACE:
					if( processFace( TopoDS::Face( subShape ), data, NULL ) )
						ret = true;
					break;

				default:
					break;
			}
		}

		return ret;
	}

	bool processNode( const TopoDS_Shape& shape, DATA& data)
	{
		TopAbs_ShapeEnum stype = shape.ShapeType();
		bool ret = true;
		std::cout << "stye " << stype << std::endl;

		switch( stype )
		{
			case TopAbs_COMPOUND:
			case TopAbs_COMPSOLID:
				if( processComp( shape, data ) )
					ret = true;
				break;

			case TopAbs_SOLID:
				if( processSolid( shape, data) )
					ret = true;
				break;

			case TopAbs_SHELL:
				if( processShell( shape, data,  NULL ) )
					ret = true;
				break;

			case TopAbs_FACE:
				if( processFace( TopoDS::Face( shape ), data, NULL ) )
					ret = true;
				break;

			default:
				break;
		}

		return ret;
	}


	std::deque<Face> import(const std::string &filename) {
		std::deque<Face> faces;
		DATA data;
		data.faces = &faces;
		Handle(XCAFApp_Application) m_app = XCAFApp_Application::GetApplication();

		m_app->NewDocument( "MDTV-XCAF", data.m_doc );
		if( !readSTEP( data.m_doc, filename.c_str() ) ) {
			std::cout<<"error" <<std::endl;
			return faces;
		}
		std::cout << "loaded" << std::endl;

		data.m_assy = XCAFDoc_DocumentTool::ShapeTool( data.m_doc->Main() );
		data.m_color = XCAFDoc_DocumentTool::ColorTool( data.m_doc->Main() );
		TDF_LabelSequence frshapes;
		data.m_assy->GetFreeShapes( frshapes );

		int nshapes = frshapes.Length();
		int id =1;
		std::cout << "shapes " << nshapes << std::endl;
		bool ret = false;
		while( id <= nshapes ) {
			TopoDS_Shape shape = data.m_assy->GetShape( frshapes.Value(id) );
			if ( !shape.IsNull() && processNode( shape, data) ) {
				ret = true;
			}
			++id;
		}

		return faces;

	}
}
