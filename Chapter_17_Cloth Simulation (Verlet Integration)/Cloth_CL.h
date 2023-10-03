

namespace ClothSimulation_OpenCL
{
    typedef struct _Cloth* Cloth;

    bool Initialize();

    Cloth CreateCloth( unsigned int cloth_width, unsigned int cloth_height, unsigned int x_vertices, unsigned int y_verticecs, float damping, float mass);
    void Render( Cloth cloth);
    void RenderVertices( Cloth cloth);
    void Update( Cloth cloth, float delta_time, vmath::vec3 bound_dimension);
    void DeleteCloth( Cloth cloth);
    
    void Uninitialize();

} // namespace ClothSimulation_OpenCL

