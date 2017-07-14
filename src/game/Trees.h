// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

//Binary Tree and Octal Tree templates.
//Ok, and a doubly linked list too, for good measure.
//By Seumas McNally.

#ifndef TREES_H
#define TREES_H

#include <new>
#include <stdint.h>

//Returns bool of whether bit is set.
inline bool TestBit(int val, int bit){
	return (val & (1 <<bit)) ? true : false;
};
//Returns val with specified bit either set or cleared.
inline int32_t SetBit(int32_t val, int32_t bit, int32_t set = 1){
	return (set ? val | (1 <<bit) : val & (~(1 <<bit)));
};

//Base class template for making inherently linklistable objects.
//USAGE: class MyClass : public LinklistBase<MyClass> { };
//Making the inherited prev and next pointers point to objects of MyClass.
//For the list head, use a single object of class LinklistBase<MyClass>.
//MyClass *MUST* *ALWAYS* derive from LinklistBase<MyClass>!
//
//Ah crud, there's a problem with making the list head a Linklistbase<MyClass>
//object, and that is that if you take the PrevLink() of the first real link,
//it will ostensibly point to a real object of MyClass, but if you try to access
//any MyClass members it will bomb!  Argh!  So the list can't be walked backwards
//without having a pointer to the head object to test links against...
//
template <class ME> class LinklistBase{
protected:
	ME *Prev;
	ME *Next;
public:
	LinklistBase() : Prev(nullptr), Next(nullptr) {};
	virtual ~LinklistBase(){
		//Deletes this item and the entire list, so Local list heads are OK, the auto-
		//destructor will erase the whole list attached to it.
		if(Next){
			Next->Prev = nullptr;	//Unlink first, so we don't get deleted again.
			delete Next;
		}
		if(Prev){
			Prev->Next = nullptr;
			delete Prev;
		}
	};
	void DeleteList(){
		//Deletes the items attached to this item (and attached to those, etc.) but
		//does NOT delete this item, unlike DeleteItem.  Use to clear the attached list
		//of a global or local list head object without touching the head.
		//Now uses a non-recursive algorithm to avoid problems with huge lists.
		//But be sure to DeleteList the head and not just auto-destruct!
		ME *Ptr, *Ptr2;
		Ptr = Next;
		while(Ptr){
			Ptr2 = Ptr;	//Copy pointer.
			Ptr = Ptr->Next;	//Next item.
			Ptr2->UnlinkItem();	//Unlink current item.
			delete Ptr2;	//Destroy unlinked item.
		}
		Ptr = Prev;
		while(Ptr){
			Ptr2 = Ptr;	//Copy pointer.
			Ptr = Ptr->Prev;	//Next item.
			Ptr2->UnlinkItem();	//Unlink current item.
			delete Ptr2;	//Destroy unlinked item.
		}
	};
	void UnlinkItem(){
		//Use this function to link and unlink items in various lists without deleting them.
		//If using a linklist child as a sub-object that will be in and out of lists, be
		//sure to call UnlinkItem on the linklist nodes in your class's destructor, and
		//never call DeleteList or delete on any member of the lists!
		if(Prev) Prev->Next = Next;
		if(Next) Next->Prev = Prev;
		Next = Prev = nullptr;
	};
	void DeleteItem(){
		//Deletes this one item and relinks list around it.  Calls destructor on "this".
		//Do NOT DeleteItem the head item in a list, or the rest of the list will be lost!
		UnlinkItem();
		delete this;
	};
	ME *FindItem(int n){	//Returns pointer to nth item in list, 0 being the item whose member you called.
		ME *Ptr;	// Russ - bug fix, was used out of scope
		if(n < 0) return 0;
		for(Ptr = (ME*)this; Ptr && n; Ptr = Ptr->Next) n--;
		return Ptr;
	};
	int CountItems(int n = 0){	//Counts items in list including head, going forwards.  To not count the head, pass in -1.
		for(ME *Ptr = (ME*)this; Ptr; Ptr = Ptr->Next) n++;
		return n;
	};
	ME *Head(){	//Finds and returns the head.
		ME *Ptr = (ME*)this;
		while(Ptr->Prev) Ptr = Ptr->Prev;
		return Ptr;
	};
	ME *Tail(){	//Finds tail (end) of list.
		ME *Ptr = (ME*)this;
		while(Ptr->Next) Ptr = Ptr->Next;
		return Ptr;
	};
	ME *AddObject(ME *item){
		//USAGE:  foo->AddItem(new MyClass(whatever));
		if(Next) return Tail()->AddObject(item);
		//Head and Tail have been optimized to non-recursive
		Next = item;
		if(Next){
			Next->Prev = (ME*)this;
			Next->Next = nullptr;
		}
		return Next;
	};
	ME *InsertObjectAfter(ME *item){
		//Inserts a list item after the current item.
		if(item){
			if(Next){
				Next->Prev = item; item->Next = Next;
			}else{
				item->Next = nullptr;
			}
			Next = item;
			item->Prev = (ME*)this;
		}
		return item;
	};
	ME *InsertObjectBefore(ME *item){
		//Inserts a list item before the current item.
		if(item){
			if(Prev){
				Prev->Next = item; item->Prev = Prev;
			}else{
				item->Prev = nullptr;
			}
			Prev = item;
			item->Next = (ME*)this;
		}
		return item;
	};
	ME *SwapWith(ME *swap){
		ME *tn, *tp;
		if(swap){
			tn = Next;
			tp = Prev;
			Next = swap->Next;
			Prev = swap->Prev;	//Ok, pointers of the two involved items are now swapped.
			swap->Next = tn;
			swap->Prev = tp;
			//Here we need an identity pointer check, for when swapping with an item right next to us.
			if(Next == (ME*)this) Next = swap;
			if(Prev == (ME*)this) Prev = swap;
			if(swap->Next == swap) swap->Next = (ME*)this;	//Might be able to optimize out this pair of checks.
			if(swap->Prev == swap) swap->Prev = (ME*)this;
			//Now reach out from their known-good pointers and redo back-links.
			if(Next) Next->Prev = (ME*)this;
			if(Prev) Prev->Next = (ME*)this;
			if(swap->Next) swap->Next->Prev = swap;
			if(swap->Prev) swap->Prev->Next = swap;
			return (ME*)this;
		}
		return 0;
	};
	ME *ShiftItemUp(){
		return SwapWith(Prev);
	};
	ME *ShiftItemDown(){
		return SwapWith(Next);
	};
	ME *NextLink(){ return Next; };
	ME *PrevLink(){ return Prev; };
};

#endif
