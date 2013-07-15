
#include "precomp.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget implementation

class cv::viz::Widget::Impl
{
public:
    vtkSmartPointer<vtkProp> prop;
    int ref_counter;
    
    Impl() : prop(0) {}
};

cv::viz::Widget::Widget() : impl_(0)
{
    create();
}

cv::viz::Widget::Widget(const Widget &other) : impl_(other.impl_) 
{
    if (impl_) CV_XADD(&impl_->ref_counter, 1);
}

cv::viz::Widget& cv::viz::Widget::operator=(const Widget &other)
{
    if (this != &other)
    {
        release();
        impl_ = other.impl_;
        if (impl_) CV_XADD(&impl_->ref_counter, 1);
    }
    return *this;
}

cv::viz::Widget::~Widget()
{
    release();
}

void cv::viz::Widget::create()
{
    if (impl_) release();
    impl_ = new Impl();
    impl_->ref_counter = 1;
}

void cv::viz::Widget::release()
{
    if (impl_ && CV_XADD(&impl_->ref_counter, -1) == 1)
    {
        delete impl_;
        impl_ = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget accessor implementaion

vtkSmartPointer<vtkProp> cv::viz::WidgetAccessor::getProp(const Widget& widget)
{
    return widget.impl_->prop;
}

void cv::viz::WidgetAccessor::setProp(Widget& widget, vtkSmartPointer<vtkProp> prop)
{
    widget.impl_->prop = prop;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget3D implementation

struct cv::viz::Widget3D::MatrixConverter
{
    static Matx44f convertToMatx(const vtkSmartPointer<vtkMatrix4x4>& vtk_matrix)
    {
        Matx44f m;
        for (int i = 0; i < 4; i++)
            for (int k = 0; k < 4; k++)
                m(i, k) = vtk_matrix->GetElement (i, k);
        return m;
    }
    
    static vtkSmartPointer<vtkMatrix4x4> convertToVtkMatrix (const Matx44f& m)
    {
        vtkSmartPointer<vtkMatrix4x4> vtk_matrix = vtkSmartPointer<vtkMatrix4x4>::New ();
        for (int i = 0; i < 4; i++)
            for (int k = 0; k < 4; k++)
                vtk_matrix->SetElement(i, k, m(i, k));
        return vtk_matrix;
    }
};

void cv::viz::Widget3D::setPose(const Affine3f &pose)
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);
    
    vtkSmartPointer<vtkMatrix4x4> matrix = convertToVtkMatrix(pose.matrix);
    actor->SetUserMatrix (matrix);
    actor->Modified ();
}

void cv::viz::Widget3D::updatePose(const Affine3f &pose)
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);
    
    vtkSmartPointer<vtkMatrix4x4> matrix = actor->GetUserMatrix();
    if (!matrix)
    {
        setPose(pose);
        return ;
    }
    Matx44f matrix_cv = MatrixConverter::convertToMatx(matrix);

    Affine3f updated_pose = pose * Affine3f(matrix_cv);
    matrix = MatrixConverter::convertToVtkMatrix(updated_pose.matrix);

    actor->SetUserMatrix (matrix);
    actor->Modified ();
}

cv::Affine3f cv::viz::Widget3D::getPose() const
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);
    
    vtkSmartPointer<vtkMatrix4x4> matrix = actor->GetUserMatrix();
    Matx44f matrix_cv = MatrixConverter::convertToMatx(matrix);
    return Affine3f(matrix_cv);
}

void cv::viz::Widget3D::setColor(const Color &color)
{
    // Cast to actor instead of prop3d since prop3d doesn't provide getproperty
    vtkActor *actor = vtkActor::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);
    
    Color c = vtkcolor(color);
    actor->GetMapper ()->ScalarVisibilityOff ();
    actor->GetProperty ()->SetColor (c.val);
    actor->GetProperty ()->SetEdgeColor (c.val);
    actor->GetProperty ()->SetAmbient (0.8);
    actor->GetProperty ()->SetDiffuse (0.8);
    actor->GetProperty ()->SetSpecular (0.8);
    actor->GetProperty ()->SetLighting (0);
    actor->Modified ();
}

template<> cv::viz::Widget3D cv::viz::Widget::cast<cv::viz::Widget3D>()
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);

    Widget3D widget;
    WidgetAccessor::setProp(widget, actor);
    return widget;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget2D implementation

void cv::viz::Widget2D::setColor(const Color &color)
{
    vtkActor2D *actor = vtkActor2D::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);
    Color c = vtkcolor(color);
    actor->GetProperty ()->SetColor (c.val);
    actor->Modified ();
}

template<> cv::viz::Widget2D cv::viz::Widget::cast<cv::viz::Widget2D>()
{
    vtkActor2D *actor = vtkActor2D::SafeDownCast(WidgetAccessor::getProp(*this));
    CV_Assert(actor);

    Widget2D widget;
    WidgetAccessor::setProp(widget, actor);
    return widget;
}
