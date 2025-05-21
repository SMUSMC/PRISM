package com.android.microdroid.prismapp;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.Toast;
import android.widget.CheckBox;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.EditText;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.content.Context;
import android.util.Log;

import java.lang.Long;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import java.io.File;
import java.io.FileOutputStream;

public class FormFragment extends Fragment {
	View view;
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
		Bundle savedInstanceState) {
		view = inflater.inflate(R.layout.fragment_form, container, false);
		final RadioButton radioacct1 = view.findViewById(R.id.radioButton);
		radioacct1.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(0);
		    }
		});
		final RadioButton radioacct2 = view.findViewById(R.id.radioButton2);
		radioacct2.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(1);
		    }
		});
		final RadioButton radioacct3 = view.findViewById(R.id.radioButton3);
		radioacct3.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(2);
		    }
		});
		
		final CheckBox check1 = view.findViewById(R.id.checkBox);
		check1.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(3);
		    }
		});
		final CheckBox check2 = view.findViewById(R.id.checkBox2);
		check2.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(4);
		    }
		});
		 
		final Button button1 = view.findViewById(R.id.button);
		button1.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(5);
		    }
		});
		final Button button2 = view.findViewById(R.id.button2);
		button2.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        processnativeclicks(6);
		    }
		});
		
		final EditText desttext = view.findViewById(R.id.editText);
		desttext.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        hideKeyboard();
		        processnativeclicks(7);
		        ((MainActivity) getActivity()).showNumPad();
		    }
		});
		final EditText amttext = view.findViewById(R.id.editText2);
		amttext.setOnClickListener(new View.OnClickListener() {
		    public void onClick(View v) {
		        hideKeyboard();
		        processnativeclicks(8);
		        ((MainActivity) getActivity()).showNumPad();
		    }
		});
		return view;
	}
	protected void hideKeyboard(){
        if (view != null) {
            InputMethodManager imm =  (InputMethodManager)((MainActivity)getActivity()).getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

    protected void processnativeclicks(int choice){
        int wid = ((MainActivity) getActivity()).getModel().protectedExecuteClick(choice);

	if (wid==7){   
	
	  ((MainActivity) getActivity()).closeForm();  
	}
    }
}
