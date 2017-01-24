//#define DLIB_JPEG_SUPPORT
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <sstream>
#include <thread>


#include <dlib/opencv.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>

#include <dlib/image_io.h>
#include <dlib/svm_threaded.h>
#include <dlib/data_io.h>
#include <dlib/image_transforms.h>
#include <dlib/gui_core.h>
#include <dlib/array2d.h>
#include <dlib/assert.h>
#include <dlib/misc_api.h>
#include <dlib/timer.h>
#include <dlib/queue.h>
#include <dlib/base64.h>



using namespace std;
using namespace cv;
using namespace cv::dnn;
using namespace dlib;


typedef dlib::array2d<hsi_pixel> image;

/* Find best class for the blob (i. e. class with maximal probability) */
void getMaxClass(dnn::Blob &probBlob, int *classId, double *classProb)
{
	Mat probMat = probBlob.matRefConst().reshape(1, 1); //reshape the blob to 1x1000 matrix
	Point classNumber;
	minMaxLoc(probMat, NULL, classProb, NULL, &classNumber);
	*classId = classNumber.x;
}
std::vector<String> readClassNames(const char *filename = "synset_words3.txt")
{
	std::vector<String> classNames;
	std::ifstream fp(filename);
	if (!fp.is_open())
	{
		std::cerr << "File with classes labels not found: " << filename << std::endl;
		exit(-1);
	}
	std::string name;
	while (!fp.eof())
	{
		std::getline(fp, name);
		if (name.length())
			classNames.push_back(name.substr(name.find(' ') + 1));
	}
	fp.close();
	return classNames;
}


/*UI */
class color_box : public draggable
{
	/*
	Here I am defining a custom drawable widget that is a colored box that
	you can drag around on the screen.  draggable is a special kind of drawable
	object that, as the name implies, is draggable by the user via the mouse.
	To make my color_box draggable all I need to do is inherit from draggable.
	*/
	unsigned char red, green, blue;

public:
	color_box(
		drawable_window& w,
		dlib::rectangle area,
		unsigned char red_,
		unsigned char green_,
		unsigned char blue_
	) :
		red(red_),
		draggable(w, MOUSE_WHEEL),
		green(green_),
		blue(blue_),
		t(*this, &color_box::action)
	{
		rect = area;

		t.set_delay_time(4);
		//t.start();
		set_draggable_area(dlib::rectangle(10, 10, 400, 400));

		// Whenever you make your own drawable widget (or inherit from any drawable widget 
		// or interface such as draggable) you have to remember to call this function to 
		// enable the events.  The idea here is that you can perform whatever setup you 
		// need to do to get your object into a valid state without needing to worry about 
		// event handlers triggering before you are ready.
		enable_events();
	}

	~color_box(
	)
	{
		// Disable all further events for this drawable object.  We have to do this 
		// because we don't want any events (like draw()) coming to this object while or 
		// after it has been destructed.
		disable_events();

		// Tell the parent window to redraw its area that previously contained this
		// drawable object.
		parent.invalidate_rectangle(rect);
	}

private:

	void draw(
		const canvas& c
	) const
	{
		if (hidden == false)
		{
			// The canvas is an object that represents a part of the parent window
			// that needs to be redrawn.  

			// The first thing I usually do is check if the draw call is for part
			// of the window that overlaps with my widget.  We don't have to do this 
			// but it is usually good to do as a speed hack.  Also, the reason
			// I don't have it set to only give you draw calls when it does indeed
			// overlap is because you might want to do some drawing outside of your
			// widget's rectangle.  But usually you don't want to do that :)
			dlib::rectangle area = c.intersect(rect);
			if (area.is_empty() == true)
				return;

			// This simple widget is just going to draw a box on the screen.   
			fill_rect(c, rect, rgb_pixel(red, green, blue));

			std::vector<point> poly;
			poly.push_back((rect.tl_corner() + rect.tr_corner()) / 2);
			poly.push_back((rect.tr_corner() + rect.br_corner()) / 2);
			poly.push_back((rect.br_corner() + rect.bl_corner()) / 2);
			poly.push_back((rect.bl_corner() + rect.tl_corner()) / 2);
			draw_solid_convex_polygon(c, poly, rgb_alpha_pixel(0, 0, 0, 70));
		}
	}

	void action(
	)
	{
		++red;
		parent.invalidate_rectangle(rect);
	}

	void on_wheel_up(
		unsigned long state
	)
	{
		if (state == base_window::NONE)
			cout << "up scroll, NONE" << endl;
		else if (state&base_window::LEFT)
			cout << "up scroll, LEFT" << endl;
		else if (state&base_window::RIGHT)
			cout << "up scroll, RIGHT" << endl;
		else if (state&base_window::MIDDLE)
			cout << "up scroll, MIDDLE" << endl;
		else if (state&base_window::SHIFT)
			cout << "up scroll, SHIFT" << endl;
		else if (state&base_window::CONTROL)
			cout << "up scroll, CONTROL" << endl;

	}

	void on_wheel_down(
		unsigned long state
	)
	{

		if (state == base_window::NONE)
			cout << "down scroll, NONE" << endl;
		else if (state&base_window::LEFT)
			cout << "down scroll, LEFT" << endl;
		else if (state&base_window::RIGHT)
			cout << "down scroll, RIGHT" << endl;
		else if (state&base_window::MIDDLE)
			cout << "down scroll, MIDDLE" << endl;
		else if (state&base_window::SHIFT)
			cout << "down scroll, SHIFT" << endl;
		else if (state&base_window::CONTROL)
			cout << "down scroll, CONTROL" << endl;

	}
	void on_window_resized()
	{
		draggable::on_window_resized();
	}
	timer<color_box> t;
};

class win : public drawable_window
{

	label lbl_last_keydown;
	label lbl_mod_shift;
	label lbl_mod_control;
	label lbl_mod_alt;
	label lbl_mod_meta;
	label lbl_mod_caps_lock;
	label lbl_mod_num_lock;
	label lbl_mod_scroll_lock;
	VideoCapture cap;
	Mat Cvframe;
	cv_image<bgr_pixel> cimg;
	array2d<rgb_pixel> displayimg;
	void on_keydown(
		unsigned long key,
		bool is_printable,
		unsigned long state
	)
	{
		if (is_printable)
			lbl_last_keydown.set_text(string("last keydown: ") + (char)key);
		else
			lbl_last_keydown.set_text(string("last keydown: nonprintable"));

		if (state&base_window::KBD_MOD_SHIFT)
			lbl_mod_shift.set_text("shift is on");
		else
			lbl_mod_shift.set_text("shift is off");

		if (state&base_window::KBD_MOD_CONTROL)
			lbl_mod_control.set_text("control is on");
		else
			lbl_mod_control.set_text("control is off");

		if (state&base_window::KBD_MOD_ALT)
			lbl_mod_alt.set_text("alt is on");
		else
			lbl_mod_alt.set_text("alt is off");


		if (state&base_window::KBD_MOD_META)
			lbl_mod_meta.set_text("meta is on");
		else
			lbl_mod_meta.set_text("meta is off");

		if (state&base_window::KBD_MOD_CAPS_LOCK)
			lbl_mod_caps_lock.set_text("caps_lock is on");
		else
			lbl_mod_caps_lock.set_text("caps_lock is off");

		if (state&base_window::KBD_MOD_NUM_LOCK)
			lbl_mod_num_lock.set_text("num_lock is on");
		else
			lbl_mod_num_lock.set_text("num_lock is off");


		if (state&base_window::KBD_MOD_SCROLL_LOCK)
			lbl_mod_scroll_lock.set_text("scroll_lock is on");
		else
			lbl_mod_scroll_lock.set_text("scroll_lock is off");

		drawable_window::on_keydown(key, is_printable, state);
	}

	void rb_click(
	)
	{
		if (rb.is_checked())
			rb.set_name("radio button checked");
		else
			rb.set_name("radio button");
		rb.set_checked();
	}

	void cb_sb_enabled(
		toggle_button&
	)
	{
		if (sb_enabled.is_checked())
		{
			sb.enable();
			lb.enable();
			b.enable();
		}
		else
		{
			lb.disable();
			sb.disable();
			b.disable();
		}

		if (sb_enabled.is_checked())
			rb.enable();
		else
			rb.disable();

		if (sb_enabled.is_checked())
			tabs.enable();
		else
			tabs.disable();

		if (sb_enabled.is_checked())
			tf.enable();
		else
			tf.disable();

		if (sb_enabled.is_checked())
			tb.enable();
		else
			tb.disable();

	}

	void cb_sb_shown(
	)
	{
		if (sb_shown.is_checked())
		{
			sb.show();
			tabs.show();
			lb.show();
		}
		else
		{
			sb.hide();
			tabs.hide();
			lb.hide();
		}
	}


	void tab_change(
		unsigned long new_idx,
		unsigned long
	)
	{
		tab_label.set_text(tabs.tab_name(new_idx));
	}

	void scroll_handler(
	)
	{
		ostringstream sout;
		sout << "scroll bar pos: " << sb.slider_pos();
		sbl.set_text(sout.str());
	}

	void scroll2_handler(
	)
	{
		sb.set_length(sb2.slider_pos());
		ostringstream sout;
		sout << "scroll bar2 pos: " << sb2.slider_pos();
		sbl2.set_text(sout.str());
		scroll_handler();
	}

	void scroll3_handler(
	)
	{
		sb.set_max_slider_pos(sb3.slider_pos());
		ostringstream sout;
		sout << "scroll bar3 pos: " << sb3.slider_pos();
		sbl3.set_text(sout.str());
		scroll_handler();
	}

	void lb_double_click(
		unsigned long
	)
	{
		dlib::queue<unsigned long>::kernel_2a_c sel;
		lb.get_selected(sel);
		sel.reset();
		while (sel.move_next())
		{
			cout << lb[sel.element()] << endl;
		}
		//message_box("list_box",lb[idx]);
	}

	void msg_box(
	)
	{
		message_box("title", "you clicked the ok button!\n HURRAY!");
	}

	static void try_this_junk(
		void* param
	)
	{
		win& p = *reinterpret_cast<win*>(param);
		put_on_clipboard(p.tf.text() + "\nfoobar");


	}

	void on_set_clipboard(
	)
	{
		create_new_thread(try_this_junk, this);
		//try_this_junk(this);
	}

	static void try_this_junk2(
		void*
	)
	{

		string temp;
		get_from_clipboard(temp);
		message_box("clipboard", temp);

	}
	void on_get_clipboard(
	)
	{
		create_new_thread(try_this_junk2, this);
	}


	void on_show_msg_click(
	)
	{
		message_box("title", "This is a test message.", *this, &win::msg_box);
	}

	void on_menu_help(
	)
	{
		message_box("About", "This is the messy dlib gui regression test program");
	}

	

public:

	~win()
	{
		close_window();
	}

	void cbox_clicked(
	)
	{
		if (cbox.is_checked())
			cbl.set_text(cbox.name() + " box is checked");
		else
			cbl.set_text("box NOT is checked");
	}


	win(
	) :
		drawable_window(true),
		lbl_last_keydown(*this),
		lbl_mod_shift(*this),
		lbl_mod_control(*this),
		lbl_mod_alt(*this),
		lbl_mod_meta(*this),
		lbl_mod_caps_lock(*this),
		lbl_mod_num_lock(*this),
		lbl_mod_scroll_lock(*this),
		b(*this),
		btn_count(*this),
		btn_get_clipboard(*this),
		btn_set_clipboard(*this),
		btn_show_message(*this),
		cb1(*this, dlib::rectangle(100, 100, 200, 200), 255, 0, 0),
		cb2(*this, dlib::rectangle(150, 150, 250, 240), 0, 255, 0),
		cbl(*this),
		cbox(*this),
		group1(*this),
		group2(*this),
		group3(*this),
		keyboard_count(1),
		keydown(*this),
		keyup(*this),
		l1(*this),
		l2(*this),
		l3(*this),
		lb(*this),
		leave_count(*this),
		left_down(*this),
		left_up(*this),
		middle_down(*this),
		middle_up(*this),
		mouse_state(*this),
		mt(*this),
		nrect(*this),
		pos(*this),
		rb(*this),
		right_down(*this),
		right_up(*this),
		sb2(*this, scroll_bar::VERTICAL),
		sb3(*this, scroll_bar::VERTICAL),
		sb_enabled(*this),
		sbl2(*this),
		sbl3(*this),
		sbl(*this),
		sb_shown(*this),
		sb(*this, scroll_bar::HORIZONTAL),
		scroll(*this),
		tab_label(*this),
		tabs(*this),
		tf(*this),
		tb(*this),
		mbar(*this),
		display(*this)
	{
		bool use_bdf_fonts = false;

		shared_ptr_thread_safe<bdf_font> f(new bdf_font);

		if (use_bdf_fonts)
		{

			ifstream fin("/home/davis/source/10x20.bdf");
			f->read_bdf_file(fin, 0xFFFF);

			mt.set_main_font(f);
		}
		//mt.hide();
		mt.set_pos(5, 200);


		lbl_last_keydown.set_text("?");
		lbl_mod_shift.set_text("?");
		lbl_mod_control.set_text("?");
		lbl_mod_alt.set_text("?");
		lbl_mod_meta.set_text("?");
		lbl_mod_caps_lock.set_text("?");
		lbl_mod_num_lock.set_text("?");
		lbl_mod_scroll_lock.set_text("?");

		lbl_last_keydown.set_pos(20, 420);
		lbl_mod_shift.set_pos(20, lbl_last_keydown.bottom() + 5);
		lbl_mod_control.set_pos(20, lbl_mod_shift.bottom() + 5);
		lbl_mod_alt.set_pos(20, lbl_mod_control.bottom() + 5);
		lbl_mod_meta.set_pos(20, lbl_mod_alt.bottom() + 5);
		lbl_mod_caps_lock.set_pos(20, lbl_mod_meta.bottom() + 5);
		lbl_mod_num_lock.set_pos(20, lbl_mod_caps_lock.bottom() + 5);
		lbl_mod_scroll_lock.set_pos(20, lbl_mod_num_lock.bottom() + 5);

		lb.set_pos(580, 200);
		lb.set_size(200, 300);
		if (use_bdf_fonts)
			lb.set_main_font(f);

		dlib::queue<string>::kernel_2a_c qos;
		string a;
		a = "Davis"; qos.enqueue(a);
		a = "king"; qos.enqueue(a);
		a = "one"; qos.enqueue(a);
		a = "two"; qos.enqueue(a);
		a = "three"; qos.enqueue(a);
		a = "yo yo yo alsdkjf asfj lsa jfsf\n this is a long phrase"; qos.enqueue(a);
		a = "four"; qos.enqueue(a);
		a = "five"; qos.enqueue(a);
		a = "six"; qos.enqueue(a);
		a = "seven"; qos.enqueue(a);
		a = "eight"; qos.enqueue(a);
		a = "nine"; qos.enqueue(a);
		a = "ten"; qos.enqueue(a);
		a = "eleven"; qos.enqueue(a);
		a = "twelve"; qos.enqueue(a);
		for (int i = 0; i < 1000; ++i)
		{
			a = "thirteen"; qos.enqueue(a);
		}
		lb.load(qos);
		lb.select(1);
		lb.select(2);
		lb.select(3);
		lb.select(5);
		lb.enable_multiple_select();
		lb.set_double_click_handler(*this, &win::lb_double_click);
		//        lb.disable_multiple_select();

		btn_show_message.set_pos(50, 350);
		btn_show_message.set_name("message_box()");
		mbar.set_number_of_menus(2);
		mbar.set_menu_name(0, "File", 'F');
		mbar.set_menu_name(1, "Help", 'H');
		mbar.menu(0).add_menu_item(menu_item_text("Open File", *this, &win::on_open_file_box_click, 'o'));
		mbar.menu(0).add_menu_item(menu_item_text("Open Existing File", *this, &win::on_open_existing_file_box_click, 'e'));
		mbar.menu(0).add_menu_item(menu_item_text("Save FIle", *this, &win::on_save_file_box_click, 's'));
		mbar.menu(0).add_menu_item(menu_item_separator());
		mbar.menu(0).add_submenu(menu_item_submenu("submenu", 'm'), submenu);
		submenu.add_menu_item(menu_item_separator());
		submenu.add_menu_item(menu_item_separator());
		submenu.add_menu_item(menu_item_text("show msg click", *this, &win::on_show_msg_click, 's'));
		submenu.add_menu_item(menu_item_text("get clipboard", *this, &win::on_get_clipboard, 'g'));
		submenu.add_menu_item(menu_item_text("set clipboard", *this, &win::on_set_clipboard, 'c'));
		submenu.add_menu_item(menu_item_separator());
		submenu.add_menu_item(menu_item_separator());
		mbar.menu(1).add_menu_item(menu_item_text("About", *this, &win::on_menu_help, 'A'));

		btn_show_message.set_click_handler(*this, &win::on_show_msg_click);
		btn_get_clipboard.set_pos(btn_show_message.right() + 5, btn_show_message.top());
		btn_get_clipboard.set_name("get_from_clipboard()");
		btn_get_clipboard.set_click_handler(*this, &win::on_get_clipboard);

		btn_get_clipboard.set_style(button_style_toolbar1());
		btn_set_clipboard.set_pos(btn_get_clipboard.right() + 5, btn_get_clipboard.top());
		btn_set_clipboard.set_name("put_on_clipboard()");
		btn_set_clipboard.set_click_handler(*this, &win::on_set_clipboard);

		nrect.set_size(700, 500);
		nrect.set_name("test widgets");
		nrect.set_pos(2, mbar.bottom() + 2);

		//throw dlib::error("holy crap batman");
		tab_label.set_pos(10, 440);

		tabs.set_click_handler(*this, &win::tab_change);
		tabs.set_pos(5, mbar.bottom() + 10);
		tabs.set_size(280, 100);
		tabs.set_number_of_tabs(3);
		tabs.set_tab_name(0, "davis");
		tabs.set_tab_name(1, "edward");
		tabs.set_tab_name(2, "king alsklsdkfj asfd");
		tabs.set_tab_group(0, group1);
		tabs.set_tab_group(1, group2);
		tabs.set_tab_group(2, group3);

		l1.set_text("group one");
		l2.set_text("group two");
		l3.set_text("group three");

		group1.add(l1, 0, 0);
		group2.add(l2, 20, 10);
		group3.add(l3, 0, 0);



		sb_enabled.set_name("enabled");
		sb_shown.set_name("shown");
		sb_shown.set_checked();
		sb_enabled.set_checked();
		sb_shown.set_click_handler(*this, &win::cb_sb_shown);
		sb_enabled.set_click_handler(*this, &win::cb_sb_enabled);

		sb_shown.set_tooltip_text("I'm a checkbox");

		rb.set_click_handler(*this, &win::rb_click);


		sb3.set_pos(440, mbar.bottom() + 10);
		sb3.set_max_slider_pos(300);
		sb3.set_slider_pos(150);
		sb3.set_length(300);
		sb2.set_pos(470, mbar.bottom() + 10);
		sb2.set_max_slider_pos(300);
		sb2.set_length(300);
		sb.set_pos(500, mbar.bottom() + 10);
		sb.set_max_slider_pos(30);
		sb.set_length(300);


		sb.set_scroll_handler(*this, &win::scroll_handler);
		sb2.set_scroll_handler(*this, &win::scroll2_handler);
		sb3.set_scroll_handler(*this, &win::scroll3_handler);
		sbl.set_pos(540, mbar.bottom() + 20);
		sbl2.set_pos(540, mbar.bottom() + 40);
		sbl3.set_pos(540, mbar.bottom() + 60);

		cbox.set_pos(300, mbar.bottom() + 30);
		cbox.set_name("davis king");
		cbox.set_click_handler(*this, &win::cbox_clicked);

		cbl.set_pos(300, cbox.get_rect().bottom() + 1);
		cbox.set_checked();
		sb_enabled.set_pos(cbox.get_rect().left(), cbox.get_rect().bottom() + 20);
		sb_shown.set_pos(sb_enabled.get_rect().left(), sb_enabled.get_rect().bottom() + 2);



		if (use_bdf_fonts)
			rb.set_main_font(f);
		rb.set_name("radio button");
		rb.set_pos(sb_shown.get_rect().left(), sb_shown.get_rect().bottom() + 2);


		cb1.set_z_order(10);
		cb2.set_z_order(20);

		pos.set_pos(50, 50);
		left_up.set_pos(50, 70);
		left_down.set_pos(50, 90);
		middle_up.set_pos(50, 110);
		middle_down.set_pos(50, 130);
		right_up.set_pos(50, 150);
		right_down.set_pos(50, 170);

		mouse_state.set_pos(50, 190);

		leave_count.set_pos(50, 210);

		scroll_count = 0;
		scroll.set_pos(50, 230);

		btn_count.set_pos(50, 250);


		keydown.set_pos(50, 270);
		keyup.set_pos(50, 290);

		tf.set_pos(50, 310);
		tf.set_text("Davis685g@");
		tf.set_width(500);
		tf.set_text_color(rgb_pixel(255, 0, 0));
		tf.set_enter_key_handler(*this, &win::on_enter_key);
		tf.set_focus_lost_handler(*this, &win::on_tf_focus_lost);

		tb.set_pos(250, 400);
		tb.set_text("initial test\nstring");
		tb.set_size(300, 300);
		tb.set_text_color(rgb_pixel(255, 0, 0));
		tb.set_enter_key_handler(*this, &win::on_enter_key);
		tb.set_focus_lost_handler(*this, &win::on_tf_focus_lost);

		

		button_count = 0;
		count = 0;
		b.set_name("button");
		b.set_pos(540, 100);
		b.set_click_handler(*this, &win::on_click);
		b.set_tooltip_text("hurray i'm a button!");
		if (use_bdf_fonts)
			b.set_main_font(f);


		set_size(815, 730);

		nrect.wrap_around(
			cbox.get_rect() +
			rb.get_rect() +
			sb_enabled.get_rect() +
			sb_shown.get_rect());

		flip = 0;
		//open_file_box(*this, &win::on_open_file);
		//open_existing_file_box(*this, &win::on_open_file);
		//save_file_box(*this, &win::on_open_file);
		img_init();
		display.set_pos(575,543);
		display.set_size(320, 240);
		//thread mthread(*this, &win::image1_handler);
		//mthread.join();
		//Mat temp;
		//temp =imread( "space_shuttle.jpg");
		//cimg = temp;
		//display.set_image(cimg);
		//display.set_image_clicked_handler(*this, &win::image1_click_handler);

		
		if (use_bdf_fonts)
		{
			tf.set_main_font(f);
			tb.set_main_font(f);
		}
		if (use_bdf_fonts)
			tabs.set_main_font(f);
		
	}
	

	
	thread image1_handler_thread()
	{
		return thread([=] { image1_handler(); });
	}
private:

	void on_open_file_box_click()
	{
		open_file_box(*this, &win::on_open_file);
	}

	void on_open_existing_file_box_click()
	{
		open_existing_file_box(*this, &win::on_open_file);
	}

	void on_save_file_box_click()
	{
		save_file_box(*this, &win::on_save_file);
	}

	void on_enter_key()
	{
		cout << "enter key pressed" << endl;
	}

	void on_tf_focus_lost()
	{
		cout << "text field/box lost focus" << endl;
	}


	void on_open_file(const std::string& file)
	{
		message_box("file opened", file);
	}

	void on_save_file(const std::string& file)
	{
		message_box("file saved", file);
	}

	void img_init()
	{
		cap.open(1);
		cap.set(CAP_PROP_FRAME_WIDTH, 320);
		cap.set(CAP_PROP_FRAME_HEIGHT, 240);
		cap >> Cvframe;
		resize(Cvframe, Cvframe, Size(320, 240));
		cimg = Cvframe;
			
		display.set_image(cimg);
		//cap.
	}

	void updateCam()
	{
		cap >> Cvframe;
		//Cvframe.resize(320, 240);
		resize(Cvframe, Cvframe, Size(320, 240));
		cimg = Cvframe;
		display.set_image(cimg);
	}

	void on_click(
	)
	{
		ostringstream sout;
		sout << "text field: " << tf.text();
		++button_count;
		btn_count.set_text(sout.str());
		updateCam();
		if (flip == 0)
		{
			flip = 1;
			lb.set_size(200, 200);
		}
		else if (flip == 1)
		{
			flip = 2;
			lb.set_size(150, 200);
		}
		else if (flip == 2)
		{
			flip = 3;
			lb.set_size(150, 300);
		}
		else
		{
			flip = 0;
			lb.set_size(200, 300);
		}
	}

	void image1_handler()
	{
		while (true)
		{
			cap >> Cvframe;
			cimg = Cvframe;
			display.set_image(cimg);
		}
	}
	button b;
	label btn_count;
	button btn_get_clipboard;
	button btn_set_clipboard;
	button btn_show_message;
	int button_count;
	color_box cb1;
	color_box cb2;
	label cbl;
	check_box cbox;
	int count;
	int flip;
	widget_group group1;
	widget_group group2;
	widget_group group3;
	int keyboard_count;
	label keydown;
	label keyup;
	label l1;
	label l2;
	label l3;
	list_box lb;
	label leave_count;
	label left_down;
	label left_up;
	label middle_down;
	label middle_up;
	label mouse_state;
	mouse_tracker mt;
	named_rectangle nrect;
	label pos;
	radio_button rb;
	label right_down;
	label right_up;
	scroll_bar sb2;
	scroll_bar sb3;
	check_box sb_enabled;
	label sbl2;
	label sbl3;
	label sbl;
	check_box sb_shown;
	scroll_bar sb;
	int scroll_count;
	label scroll;
	label tab_label;
	tabbed_display tabs;
	text_field tf;
	text_box tb;
	menu_bar mbar;
	popup_menu submenu;
	image_display display;
	


};

class image_process
{
	
public:
	image_process()
	{

	}

	~image_process()
	{

	}

	
private:
	Mat frame;
	cv_image<bgr_pixel> cframe;
}
;
int main(int argc, char **argv)
{
	try
	{
		/**
		String modelTxt = "D:\\project\\C\\models\\deploy_gender.prototxt";
		String modelBin = "D:\\project\\C\\models\\gender_net.caffemodel";
		String imageFile = (argc > 1) ? argv[1] : "D:\\project\\C\\models\\face.jpg";//space_shuttle.jpg";
		String imageFile2 = (argc > 1) ? argv[1] : "D:\\project\\C\\models\\face2.jpg";//space_shuttle.jpg";
		Ptr<dnn::Importer> importer;
		try                                     //Try to import Caffe GoogleNet model
		{
			importer = dnn::createCaffeImporter(modelTxt, modelBin);
		}
		catch (const cv::Exception &err)        //Importer can throw errors, we will catch them
		{
			std::cerr << err.msg << std::endl;
		}
		if (!importer)
		{
			std::cerr << "Can't load network by using the following files: " << std::endl;
			std::cerr << "prototxt:   " << modelTxt << std::endl;
			std::cerr << "caffemodel: " << modelBin << std::endl;
			std::cerr << "bvlc_googlenet.caffemodel can be downloaded here:" << std::endl;
			std::cerr << "http://dl.caffe.berkeleyvision.org/bvlc_googlenet.caffemodel" << std::endl;
			exit(-1);
		}
		dnn::Net net;
		importer->populateNet(net);
		importer.release();   //We don't need importer anymore
		namedWindow("Opencvface", WINDOW_AUTOSIZE);
		**/
		win *form = new win();
		form->set_pos(100, 200);
		form->set_title("test window");
		form->show();

		thread mthread = form->image1_handler_thread();
		mthread.join();
		//form.video_play();
		//VideoCapture cap(1);
		//VideoCapture cap("rtsp://10.4.17.155//live2.sdp");
		//VideoCapture cap("rtsp://admin:123456@192.168.1.6:7070/tracker1");
		VideoCapture cap("d:/video/20160109.mp4");
		//dlib::array<array2d<unsigned char> > images_train, images_test;
		//std::vector<std::vector<dlib::rectangle> > face_boxes_train, face_boxes_test;
		//load_image_dataset();
		//VideoCapture cap("d:/video/Cross/1221/face_CH_003_0001_cut.avi");
	

		if (!cap.isOpened())
		{//check if we succeeded
			cerr << "Unable to connect to camera" << endl;
			return -1;
		}

		Mat img;
		Mat resizeIMG;
		image_window win;
		//image_window win2;
		//namedWindow("OHOG", WINDOW_AUTOSIZE);
		win.set_title("Fine tune");
		//win2.set_title("HOG");
		cap.set(CAP_PROP_FRAME_WIDTH, 1024);
		cap.set(CAP_PROP_FRAME_HEIGHT, 768);
		cap.set(CAP_PROP_FPS, 60);
		double A = cap.get(CAP_PROP_FRAME_WIDTH);
		double B = cap.get(CAP_PROP_FRAME_HEIGHT);
		//string C = A + " x " + B;
		cout << A << " X " << B << endl;
		cout << CAP_PROP_FPS << endl;
		//win.set_title(C);
		//win2.set_title("Just Shapes");
		// Load face detection and pose estimation models.
		frontal_face_detector detector = get_frontal_face_detector();
		shape_predictor pose_model;
		deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

		array2d<matrix<float, 31, 1> > hog;
		cout << "hog image has " << hog.nr() << " rows and " << hog.nc() << " columns." << endl;
		//std::vector<String> classNames;
		dlib::array<array2d<unsigned char> > images_train, images_test;
		std::vector<std::vector<dlib::rectangle> > face_boxes_train, face_boxes_test;

		time_t nStart;
		time_t nEnd;
		point p;
		for (; ; )
		{
			nStart = time(NULL);
			//cap.set(CAP_PROP_FRAME_HEIGHT = 480, CAP_PROP_FRAME_WIDTH = 640);
			
			//cap.set(3, 640);
			cap >> img;
			resize(img, resizeIMG, Size(640, 480));
			//cv_image<bgr_pixel> cimg(img);
			cv_image<bgr_pixel> cimg(resizeIMG);
			//array2d<matrix<float, 31, 1> > hog;
			extract_fhog_features(cimg, hog);
			//imshow("img", img);
			//waitKey(30);
			int classId;
			double classProb;


			// Detect faces 
			std::vector<dlib::rectangle> faces = detector(cimg);
			// Find the pose of each face.
			std::vector<full_object_detection> shapes;
			for (unsigned long i = 0; i < faces.size(); ++i)
			{
				shapes.push_back(pose_model(cimg, faces[i]));
			}
			// Display it all on the screen
			win.clear_overlay();
			win.set_image(cimg);
			win.add_overlay(faces);
			win.add_overlay(render_face_detections(shapes));
			//Sleep(10);
			//dlib::array< array2d<rgb_pixel> > face_chips;
			//extract_image_chips(img, get_face_chip_details(shapes), face_chips);
			 //win2.set_image(tile_images(face_chips));
			//win2.clear_overlay();


			//win2.set_image(draw_fhog(hog));



			//point p;  // A 2D point, used to represent pixel locations.
		//	while (win.get_next_double_click(p))
			{
				//	point hp = image_to_fhog(p);
				//	cout << "The point " << p << " in the input image corresponds to " << hp << " in hog space." << endl;
				//	cout << "FHOG features at this point: " << trans(hog[hp.y()][hp.x()]) << endl;
			}

			//win2.add_overlay(shapes);
			//waitKey(30);
			//bgr_pixel cimg2;
			//cimg2 = cimg.
			//cimg3= cimg.v
			//Mat cvImg1 = toMat(cimg);

			//Mat cvImg2 = toMat(render_face_detections(shapes));
			//cvImg2.copyTo(cvImg1);

			//imshow("Opencvface", cvImg1);
			//waitKey(10);
			//if (i % 2 == 1)
			{
				//img = imread(imageFile);
			}
			//else
			{
				//img = imread(imageFile2);
			}

			if (img.empty())
			{
				std::cerr << "Can't read image from the file: " << "imageFile" << std::endl;
				exit(-1);
			}

			//resize(img, img, Size(256, 256));       //GoogLeNet accepts only 224x224 RGB-images
			//imshow("face", img);
			//waitKey(10);
			//Sleep(10);
			/**
			dnn::Blob inputBlob = dnn::Blob(img);   //Convert Mat to dnn::Blob image batch
			net.setBlob(".data", inputBlob);        //set the network input
			net.forward();                          //compute output
			dnn::Blob prob = net.getBlob("prob");   //gather output of "prob" layer
			getMaxClass(prob, &classId, &classProb);//find the best class
			classNames = readClassNames();
			std::cout << "Best class: #" << classId << " '" << classNames.at(classId) << "'" << std::endl;
			std::cout << "Probability: " << classProb * 100 << "%" << std::endl;
			nEnd = time(NULL);
			std::cout << "Caculate time: " << nEnd - nStart << std::endl;
			**/

			//if (waitKey(30) >= 0)
			{
				//break;
			}
		}
		//waitKey(0);
	
		form->wait_until_closed();
		return 0;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
		sleep(100000);
	}
} //main