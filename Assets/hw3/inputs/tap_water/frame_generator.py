frames = list(range(0,286))
template = """\
<Scene>
    <MaxRecursionDepth>6</MaxRecursionDepth>
    
    <BackgroundColor>0 0 0</BackgroundColor>

    <Cameras>
        <Camera id="1">
            <Position>2 -6 1</Position>
            <Gaze>-0.2 1 0</Gaze>
            <Up>0 0 1</Up>
            <NearPlane>-0.9 0.9 -0.9 0.9</NearPlane>
            <NearDistance>3</NearDistance>
            <ImageResolution>1000 1000</ImageResolution>
            <ImageName>tap_{}.png</ImageName>
            <NumSamples>64</NumSamples>
        </Camera>
    </Cameras>

    <Lights>
        <AmbientLight>25.4 25.5 23.3</AmbientLight>
        <PointLight id="1">
            <Position>0 2 4</Position>
            <Intensity>200000 200000 200000</Intensity>
        </PointLight>
    </Lights>

    <Materials>
        <Material id="1">
            <AmbientReflectance>1 1 1</AmbientReflectance>
            <DiffuseReflectance>0.08 0.08 0.08</DiffuseReflectance>
            <SpecularReflectance>0 0 0</SpecularReflectance>
        </Material>
        <Material id="2">
            <AmbientReflectance>0.5 0.5 0.5</AmbientReflectance>
            <DiffuseReflectance>0 0 0.001</DiffuseReflectance>
            <SpecularReflectance>0 0 0</SpecularReflectance>
        </Material>
        <Material id="3" type="conductor">
            <AmbientReflectance>1 1 1</AmbientReflectance>
            <DiffuseReflectance>0 0 0</DiffuseReflectance>
            <SpecularReflectance>0 0 0</SpecularReflectance>
            <MirrorReflectance>1 0.86 0.57</MirrorReflectance>
            <RefractionIndex>0.370</RefractionIndex>
            <AbsorptionIndex>2.820</AbsorptionIndex>
            <Roughness>0.25</Roughness>
        </Material>
        <Material id="4" type="dielectric">
            <AmbientReflectance>0 0 0</AmbientReflectance>
            <DiffuseReflectance>0 0 0</DiffuseReflectance>
            <SpecularReflectance>0 0 0</SpecularReflectance>
            <AbsorptionCoefficient>0.01 0.01 0.01</AbsorptionCoefficient>
            <RefractionIndex>1.55</RefractionIndex>
        </Material>
    </Materials>

    <VertexData>
        -12 -10 10
        10 -10 10
        10 10 10
        -12 10 10
        -12 -10 -10
        10 -10 -10
        10 10 -10
        -12 10 -10
    </VertexData>

    <Objects>
        <Mesh id="1">
            <Material>1</Material>
            <Faces>
                1 2 6
                6 5 1
            </Faces>
        </Mesh>
        <Mesh id="2">
            <Material>1</Material>
            <Faces>
                5 6 7
                7 8 5
            </Faces>
        </Mesh>
        <Mesh id="3">
            <Material>1</Material>
            <Faces>
                7 3 4
                4 8 7
            </Faces>
        </Mesh>
        <Mesh id="4">
            <Material>2</Material>
            <Faces>
                8 4 1
                8 1 5
            </Faces>
        </Mesh>
        <Mesh id="5">
            <Material>1</Material>
            <Faces>
                2 3 7
                2 7 6
            </Faces>
        </Mesh>
        <Mesh id="6">
            <Material>1</Material>
            <Faces>
                1 3 2
                1 4 3
            </Faces>
        </Mesh>
        <Mesh id="7" shadingMode="smooth">
            <Material>3</Material>
            <Faces plyFile="../ply/tap_tap_fixed.ply" />
        </Mesh>
        <Mesh id="8" shadingMode="smooth">
            <Material>4</Material>
            <Faces plyFile="../ply/tap_water_{}_fixed.ply" />
        </Mesh>
    </Objects>
</Scene>

"""
for i in frames:
    file_name = "tap_{}.xml".format(str(i).zfill(4));
    with open(file_name,"w") as f:
        f.write(template.format(str(i).zfill(4),i))

