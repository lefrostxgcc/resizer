#include <gtk/gtk.h>
#include <math.h>

#define	SPACING	5

struct ch_color
{
	guchar r;
	guchar g;
	guchar b;
	guchar a;
};

static void on_button_load_image_clicked(GtkWidget *button, gpointer data);
static void on_button_change_image_clicked(GtkWidget *button, gpointer data);
static void load_image(const gchar *filename);
static void resize_nearest(void);
static void resize_bilinear(void);
static void bilinear(GdkPixbuf *pixbuf, double x, double y, int x1, int y1,
	struct ch_color *result);
static int bilinear_color(double x, double y, int x1, int y1,
	int Q11, int Q12, int Q21, int Q22);
static void get_pixel(GdkPixbuf *pixbuf, int x, int y, struct ch_color *color);
static void set_pixel(GdkPixbuf *pixbuf, int x, int y, struct ch_color *color);

static GdkPixbuf	*last_load_image_pixbuf;
static GtkWidget	*window;
static GtkWidget	*image;
static GtkWidget	*entry_image_filename;
static GtkWidget	*scale_resize;
static GtkWidget	*combo_box_resize;

static char *resize_names[] =
{
	"Метод ближайшего соседа",
	"Билинейная интерполяция"
};

static const int resize_names_size =
	sizeof resize_names / sizeof resize_names[0];

int main(int argc, char *argv[])
{
	GtkWidget		*vbox;
	GtkWidget		*hbox;
	GtkWidget		*frame_image;
	GtkWidget		*button_load_image;
	GtkWidget		*label_resize;
	GtkWidget		*label_algorithm;
	GtkWidget		*button_change_image;
	GtkListStore	*store_resize;
	GtkCellRenderer	*render;
	GtkTreeIter		iter;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),
		"Изменение размера изображений");
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING);
	frame_image = gtk_frame_new(NULL);
	image = gtk_image_new();
	button_load_image = gtk_button_new_with_label("Загрузить картинку");
	entry_image_filename = gtk_entry_new();
	label_resize = gtk_label_new("Масштаб: (%)");
	scale_resize = gtk_scale_new_with_range(
		GTK_ORIENTATION_HORIZONTAL, 10, 20000, 1);
	gtk_range_set_value(GTK_RANGE(scale_resize), 100);
	label_algorithm = gtk_label_new("Алгоритм:");
	button_change_image = gtk_button_new_with_label("Изменить картинку");

	gtk_widget_set_valign(button_change_image, GTK_ALIGN_END);

	store_resize = gtk_list_store_new(1, G_TYPE_STRING);
	for (int i = 0; i < resize_names_size; i++)
	{
		gtk_list_store_append(store_resize, &iter);
		gtk_list_store_set(store_resize, &iter, 0, resize_names[i], -1);
	}

	combo_box_resize =
		gtk_combo_box_new_with_model(GTK_TREE_MODEL(store_resize));
	g_object_unref(store_resize);

	render = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box_resize),
		render, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box_resize),
		render,
		"text", 0,
		NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box_resize), 0);

	load_image("/home/chip/Pictures/beach.png");

	gtk_container_set_border_width(GTK_CONTAINER(window), SPACING);
	gtk_widget_set_sensitive(entry_image_filename, FALSE);
	gtk_container_add(GTK_CONTAINER(frame_image), image);
	gtk_box_pack_start(GTK_BOX(vbox), button_load_image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), entry_image_filename, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label_resize, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scale_resize, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label_algorithm, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), combo_box_resize, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), button_change_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), frame_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), hbox);
	g_signal_connect(G_OBJECT(button_load_image), "clicked",
		G_CALLBACK(on_button_load_image_clicked), NULL);
	g_signal_connect(G_OBJECT(button_change_image), "clicked",
		G_CALLBACK(on_button_change_image_clicked), NULL);
	g_signal_connect(G_OBJECT(window), "destroy",
		G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show_all(window);
	gtk_main();
}

static void on_button_load_image_clicked(GtkWidget *button, gpointer data)
{
	GtkWidget	*dialog;
	gchar		*filename;

	dialog = gtk_file_chooser_dialog_new(NULL, GTK_WINDOW(window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		"Cancel", GTK_RESPONSE_CANCEL,
		"Open", GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
		g_get_home_dir());

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		load_image(filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

static void on_button_change_image_clicked(GtkWidget *button, gpointer data)
{
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box_resize)) == 0)
		resize_nearest();
	else if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box_resize)) == 1)
		resize_bilinear();
}

static void load_image(const gchar *filename)
{
	gtk_image_set_from_file(GTK_IMAGE(image), filename);
	gtk_entry_set_text(GTK_ENTRY(entry_image_filename), filename);
	if (last_load_image_pixbuf)
		g_object_unref(last_load_image_pixbuf);
	last_load_image_pixbuf = gdk_pixbuf_copy(
		gtk_image_get_pixbuf(GTK_IMAGE(image)));
}

static void resize_nearest(void)
{
	struct ch_color	image_color;
	struct ch_color	result_color;
	GdkPixbuf		*image_pixbuf;
	GdkPixbuf		*result_pixbuf;
	double			resize_coeff;
	double			width_resize_coeff;
	double			height_resize_coeff;
	double			row0f;
	double			col0f;
	int				row;
	int				col;
	int				row0;
	int				col0;
	int				old_width;
	int				old_height;
	int				new_width;
	int				new_height;

	image_pixbuf = last_load_image_pixbuf;
	resize_coeff = gtk_range_get_value(GTK_RANGE(scale_resize)) / 100.0;
	old_width = gdk_pixbuf_get_width(image_pixbuf);
	old_height = gdk_pixbuf_get_height(image_pixbuf);
	new_width = old_width * resize_coeff;
	new_height = old_height * resize_coeff;
	width_resize_coeff = (double)(old_width - 1) / (new_width - 1);
	height_resize_coeff = (double)(old_height - 1) / (new_height - 1);
	result_pixbuf = gdk_pixbuf_new(
				GDK_COLORSPACE_RGB,
                gdk_pixbuf_get_has_alpha(image_pixbuf),
                gdk_pixbuf_get_bits_per_sample(image_pixbuf),
                new_width,
                new_height);

	for (row = 0; row < new_height; row++)
	{
		row0f = row * height_resize_coeff;
		row0 = row0f;
		if (row0f - row0 > 0.5)
			row0++;
		for (col = 0; col < new_width; col++)
		{
			col0f = col * width_resize_coeff;
			col0 = col0f;
			if (col0f - col0 > 0.5)
				col0++;
			get_pixel(image_pixbuf, col0, row0, &image_color);
			result_color = image_color;
			set_pixel(result_pixbuf, col, row, &result_color);
		}
	}
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), result_pixbuf);
	g_object_unref(result_pixbuf);
}

static void resize_bilinear(void)
{
	struct ch_color	result_color;
	GdkPixbuf		*image_pixbuf;
	GdkPixbuf		*result_pixbuf;
	double			resize_coeff;
	double			width_resize_coeff;
	double			height_resize_coeff;
	double			x;
	double			y;
	int				new_y;
	int				new_x;
	int				x1;
	int				y1;
	int				old_width;
	int				old_height;
	int				new_width;
	int				new_height;

	image_pixbuf = last_load_image_pixbuf;
	resize_coeff = gtk_range_get_value(GTK_RANGE(scale_resize)) / 100.0;
	old_width = gdk_pixbuf_get_width(image_pixbuf);
	old_height = gdk_pixbuf_get_height(image_pixbuf);
	new_width = old_width * resize_coeff;
	new_height = old_height * resize_coeff;
	width_resize_coeff = (double)(old_width - 1) / (new_width - 1);
	height_resize_coeff = (double)(old_height - 1) / (new_height - 1);

	result_pixbuf = gdk_pixbuf_new(
				GDK_COLORSPACE_RGB,
                gdk_pixbuf_get_has_alpha(image_pixbuf),
                gdk_pixbuf_get_bits_per_sample(image_pixbuf),
                new_width,
                new_height);

	for (new_y = 0; new_y < new_height; new_y++)
	{
		y = new_y * height_resize_coeff;
		y1 = floor(y);
		if (y1 > old_height - 2)
			y1 = old_height - 2;
		for (new_x = 0; new_x < new_width; new_x++)
		{
			x = new_x * width_resize_coeff;
			x1 = floor(x);
			if (x1 > old_width - 2)
				x1 = old_width - 2;
			bilinear(image_pixbuf, x, y, x1, y1, &result_color);
			set_pixel(result_pixbuf, new_x, new_y, &result_color);
		}
	}
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), result_pixbuf);
	g_object_unref(result_pixbuf);
}

static void bilinear(GdkPixbuf *pixbuf, double x, double y, int x1, int y1,
	struct ch_color *result)
{
	struct ch_color	Q11;
	struct ch_color	Q12;
	struct ch_color	Q21;
	struct ch_color	Q22;
	int				x2;
	int				y2;

	x2 = x1 + 1;
	y2 = y1 + 1;
	get_pixel(pixbuf, x1, y1, &Q11);
	get_pixel(pixbuf, x1, y2, &Q12);
	get_pixel(pixbuf, x2, y1, &Q21);
	get_pixel(pixbuf, x2, y2, &Q22);

	result->r = bilinear_color(x, y, x1, y1, Q11.r, Q12.r, Q21.r, Q22.r);
	result->g = bilinear_color(x, y, x1, y1, Q11.g, Q12.g, Q21.g, Q22.g);
	result->b = bilinear_color(x, y, x1, y1, Q11.b, Q12.b, Q21.b, Q22.b);
	result->a = bilinear_color(x, y, x1, y1, Q11.a, Q12.a, Q21.a, Q22.a);
}

static int bilinear_color(double x, double y, int x1, int y1,
	int Q11, int Q12, int Q21, int Q22)
{
	int		R1;
	int		R2;
	int		P;
	int		x2;
	int		y2;

	x2 = x1 + 1;
	y2 = y1 + 1;
	R1 = (x2 - x) * Q11 + (x - x1) * Q21;
	R2 = (x2 - x) * Q12 + (x - x1) * Q22;
	P = (y2 - y) * R1 + (y - y1) * R2;
	if (P < 0)
		P = 0;
	else if (P > 255)
		P = 255;
	return P;
}

static void get_pixel(GdkPixbuf *pixbuf, int x, int y, struct ch_color *color)
{
	guchar	*pixels;
	guchar	*p;
	int		width;
	int		height;
	int		rowstride;
	int		n_channels;

	n_channels = gdk_pixbuf_get_n_channels(pixbuf);
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	if (x < 0 || x > width || y < 0 || y > height)
		return;

	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	p = pixels + y * rowstride + x * n_channels;
	color->r		= p[0];
	color->g	= p[1];
	color->b 	= p[2];
	if (gdk_pixbuf_get_has_alpha(pixbuf))
		color->a = p[3];
}

static void set_pixel(GdkPixbuf *pixbuf, int x, int y, struct ch_color *color)
{
	guchar	*pixels;
	guchar	*p;
	int		width;
	int		height;
	int		rowstride;
	int		n_channels;

	n_channels = gdk_pixbuf_get_n_channels(pixbuf);
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	if (x < 0 || x > width || y < 0 || y > height)
		return;

	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	p = pixels + y * rowstride + x * n_channels;
	p[0] = color->r;
	p[1] = color->g;
	p[2] = color->b;
	if (gdk_pixbuf_get_has_alpha(pixbuf))
		p[3] = color->a;
}
