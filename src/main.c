#include <gtk/gtk.h>

#define	SPACING	5

struct ch_color
{
	guchar red;
	guchar green;
	guchar blue;
	guchar alpha;
};

static void on_button_load_image_clicked(GtkWidget *button, gpointer data);
static void on_button_change_image_clicked(GtkWidget *button, gpointer data);
static void load_image(const gchar *filename);
static void change_image(void);
static void get_pixel(GdkPixbuf *pixbuf, int x, int y, struct ch_color *color);
static void set_pixel(GdkPixbuf *pixbuf, int x, int y, struct ch_color *color);

static GdkPixbuf	*last_load_image_pixbuf;
static GtkWidget	*window;
static GtkWidget	*image;
static GtkWidget	*entry_image_filename;
static GtkWidget	*scale_resize;

int main(int argc, char *argv[])
{
	GtkWidget		*vbox;
	GtkWidget		*hbox;
	GtkWidget		*frame_image;
	GtkWidget		*button_load_image;
	GtkWidget		*label_resize;
	GtkWidget		*button_change_image;

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
		GTK_ORIENTATION_HORIZONTAL, 10, 190, 1);
	gtk_range_set_value(GTK_RANGE(scale_resize), 100);
	button_change_image = gtk_button_new_with_label("Изменить картинку");

	gtk_widget_set_valign(button_change_image, GTK_ALIGN_END);

	load_image("/home/chip/Pictures/beach.png");

	gtk_container_set_border_width(GTK_CONTAINER(window), SPACING);
	gtk_widget_set_sensitive(entry_image_filename, FALSE);
	gtk_container_add(GTK_CONTAINER(frame_image), image);
	gtk_box_pack_start(GTK_BOX(vbox), button_load_image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), entry_image_filename, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label_resize, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scale_resize, FALSE, FALSE, 0);
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
	change_image();
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

static void change_image(void)
{
	struct ch_color	image_color;
	struct ch_color	result_color;
	GdkPixbuf		*image_pixbuf;
	GdkPixbuf		*result_pixbuf;
	int				row;
	int				col;
	int				width;
	int				height;

	image_pixbuf = last_load_image_pixbuf;
	result_pixbuf = gdk_pixbuf_copy(image_pixbuf);
	width = gdk_pixbuf_get_width(image_pixbuf);
	height = gdk_pixbuf_get_height(image_pixbuf);
	for (row = 0; row < height; row++)
		for (col = 0; col < width; col++)
		{
			get_pixel(image_pixbuf, col, row, &image_color);
			result_color = image_color;
			set_pixel(result_pixbuf, col, row, &result_color);
		}
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), result_pixbuf);
	g_object_unref(result_pixbuf);
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
	color->red		= p[0];
	color->green	= p[1];
	color->blue 	= p[2];
	if (gdk_pixbuf_get_has_alpha(pixbuf))
		color->alpha = p[3];
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
	p[0] = color->red;
	p[1] = color->green;
	p[2] = color->blue;
	if (gdk_pixbuf_get_has_alpha(pixbuf))
		p[3] = color->alpha;
}
